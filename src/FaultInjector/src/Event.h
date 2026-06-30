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
#include <optional>
#include <ostream>
#include <string>

#include "vpi_user.h"

namespace fin {

struct Event {
    enum class Type : std::uint8_t {
        SingleEventTransientUpset,
        SingleEventTransientRollback,
        SingleEventUpset,
    };

    vpiHandle vpi_handle{};
    std::string sig_path{};
    int time{};
    int bit_idx{};
    Type type{};
    std::optional<s_vpi_value> vpi_value{};

    bool operator<(const Event& other) const { return time < other.time; }

    friend std::ostream& operator<<(std::ostream& os, const Event& ev) {
        return os << "{[" << ev.time << "] " << ev.sig_path << "(" << ev.bit_idx << ")}";
    }
};

std::optional<Event> parseEvent(const std::string& line);

}  // namespace fin
