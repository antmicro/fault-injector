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

#include "GlobalOpts.h"

#include <ostream>

struct Cell {
    std::string name;
    std::string type;

    friend std::ostream& operator<<(std::ostream& os, const Cell& cell) {
        return os << "{ .name=" << cell.name << ", .type=" << cell.type << " }";
    }
};


class IsFlipFlop {
   public:
    static bool check(const Cell& json);
    static void ctor(const GlobalOpts& opts);
};
