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
#include "FaultEvent.h"
#include "FaultStrategy.h"
#include "Signal.h"

#include <random>
#include <span>
#include <vector>

struct WeibullConfig {
    struct Stream {
        double let;       // [Mev * cm^2 / mg]
        double flux_phi;  // [s^-1 * cm^-2]
        double max_time;  // [s]
    };
    std::vector<Stream> streams;
    std::uint32_t bit_count = 256000 * 16;  // 4MB
    double cell_area = 0.25e-6;             // [mm^2]
};

class WeibullStrategy : public FaultStrategy {
   public:
    const WeibullConfig weibullConfig;

    struct RandomGen {
        explicit RandomGen(std::uint32_t seed) : random_generator(seed) {}
        std::mt19937 random_generator;
    };

   private:
    RandomGen gen{config.seed};
    std::uniform_real_distribution<double> real_dist;
    std::uniform_int_distribution<std::uint32_t> bit_dist{0, seu::MAX_BITS};

   public:
    explicit WeibullStrategy(const Config&, const WeibullConfig&);
    std::vector<FaultEvent> generate(std::span<const Signal>) override;
    std::vector<FaultEvent> generate(const WeibullConfig::Stream&, std::span<const Signal>);

    std::shared_ptr<FaultStrategy> copy_with(FaultStrategy::Config) override;
};
