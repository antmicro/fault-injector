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

#include "PlacementInfo.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

enum class SignalType {
    UNKNOWN = 0,
    WIRE,
    REGISTER,
};

inline std::ostream& operator<<(std::ostream& os, const SignalType& type) {
    switch (type) {
        case SignalType::REGISTER:
            return os << "register";
        case SignalType::WIRE:
            return os << "wire";
        default:
            return os << "unknown";
    }
}

struct Signal {
    std::string signal_path;
    std::string cell_type;
    std::uint32_t num_of_bits = 0;
    std::optional<double> area;
    std::optional<Placement> cell_placement;
    SignalType type = SignalType::UNKNOWN;

    friend std::ostream& operator<<(std::ostream& os, const Signal& signal) {
        os << "{ .signal_path=" << signal.signal_path << ", .num_of_bits=" << signal.num_of_bits
           << ", .cell_type=" << signal.cell_type << ", area=";
        if (signal.area) {
            os << signal.area.value();
        } else {
            os << "nullopt";
        }
        os << ", placement=";
        if (signal.area) {
            os << signal.cell_placement.value();
        } else {
            os << "nullopt";
        }
        return os << ", .type=" << signal.type << " }";
    }
};
