// Copyright 2026 Antmicro <antmicro.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Constants.h"
#include "FaultStrategy.h"

struct BendelConfig {
    struct Stream {
        std::string name = "";  // Stream name
        double A = 2.4704718;   // Limiting cross-section [MeV]
        double B = 0.7964778;   // [MeV]
        double energy;          // Proton energy [MeV]
        double flux_phi;        // [s^-1 * cm^-2]
        double fluence;
    };
    std::uint32_t bit_count = 1 << 24;                  // 16MB
    double device_area = 5.6 /*[mm]*/ * 10.2 /*[mm]*/;  // [mm^2]
    int num_cells = 64;
    std::vector<Stream> streams;
};

class BendelStrategy : public FaultStrategy {
    struct Cell {
        const double area;
    };
    std::uniform_real_distribution<double> real_dist;
    std::uniform_int_distribution<std::uint32_t> bit_dist{0, seu::MAX_BITS};

   public:
    const BendelConfig bendel_config;
    explicit BendelStrategy(const Config&, const BendelConfig&);
    std::vector<FaultEvent> generate(std::span<const Signal> signals) override;
    std::vector<FaultEvent> generate(const BendelConfig::Stream&, std::span<const Signal>);
    std::shared_ptr<FaultStrategy> copy_with(FaultStrategy::Config) override;
    double eventTime(const Cell& cell, const BendelConfig::Stream& stream);
};
