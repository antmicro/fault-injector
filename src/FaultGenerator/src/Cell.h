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

#include <ostream>

#include <nlohmann/json.hpp>

struct Cell {
    std::string name;
    std::string type;

    Cell(const std::string& name, const nlohmann::json& cell_json)
        : name(name), type(cell_json.value("type", "")) {}

    /* Returns signal path, in current (yosys dependent) implementation,
     * by taking prefix before first '$'.
     */
    inline std::string getPath() const { return name.substr(0, name.find('$')); }

    friend std::ostream& operator<<(std::ostream& os, const Cell& cell) {
        return os << "{ .name=" << cell.name << ", .type=" << cell.type << " }";
    }
};
