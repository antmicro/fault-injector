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

#include <cstdint>
#include <ostream>
#include <string>

enum class FaultEventType : std::uint8_t {
    UNKNOWN = 0,
    SINGLE_EVENT_TRANSIENT,
    SINGLE_EVENT_UPSET,
};

inline std::ostream& operator<<(std::ostream& os, FaultEventType type) {
    switch (type) {
        case FaultEventType::SINGLE_EVENT_TRANSIENT:
            return os << "set";
        case FaultEventType::SINGLE_EVENT_UPSET:
            return os << "seu";
        default:
            break;
    }
    return os << "unknown";
}

struct FaultEvent {
    std::uint64_t time = 0;
    std::string signal_path;
    std::uint32_t bit_index = 0;
    FaultEventType type = FaultEventType::UNKNOWN;
    bool operator<(const FaultEvent& other) const { return time < other.time; }
};
