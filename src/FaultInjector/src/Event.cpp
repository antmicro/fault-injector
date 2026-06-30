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

#include "Event.h"

#include <sstream>
#include <string>

namespace fin {

std::optional<Event> parseEvent(const std::string& line) {
    std::stringstream is(line);
    std::string time_str;
    std::getline(is, time_str, ',');
    std::string sig_path;
    std::getline(is, sig_path, ',');
    std::string bit_idx_str;
    std::getline(is, bit_idx_str, ',');
    std::string type_str;
    std::getline(is, type_str, ',');

    if (time_str.empty() || sig_path.empty() || bit_idx_str.empty() || type_str.empty()) {
        return std::nullopt;
    }

    Event::Type type;
    if (type_str == "set") {
        type = Event::Type::SingleEventTransientUpset;
    } else if (type_str == "seu") {
        type = Event::Type::SingleEventUpset;
    } else {
        return std::nullopt;
    }

    int time = 0;
    int bit_idx = 0;
    try {
        time = std::stoi(time_str);
        bit_idx = std::stoi(bit_idx_str);
    } catch (...) {
        return std::nullopt;
    }

    return Event{
        .vpi_handle = nullptr,
        .sig_path = sig_path,
        .time = time,
        .bit_idx = bit_idx,
        .type = type,
    };
}

}  // namespace fin
