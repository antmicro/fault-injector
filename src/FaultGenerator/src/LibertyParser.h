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

#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

struct FlipFlopInfo {
    friend std::ostream& operator<<(std::ostream& os, const FlipFlopInfo&) { return os << "{}"; }
};

struct CellInfo {
    std::optional<double> area;
    std::optional<FlipFlopInfo> ff_info;

    friend std::ostream& operator<<(std::ostream& os, const CellInfo& cell) {
        os << "{ .area=";
        if (cell.area) {
            os << cell.area.value();
        } else {
            os << "nullopt";
        }
        os << ", .ff_info=";
        if (cell.ff_info) {
            os << cell.ff_info.value();
        } else {
            os << "nullopt";
        }
        return os << " }";
    }
};

struct GlobalInfo {};

struct LibertyInfo {
    GlobalInfo globalInfo;
    std::map<std::string, CellInfo> cells;
};

class LibertyParser final {
   public:
    static std::optional<LibertyInfo> parse(const std::string&);
    static std::vector<LibertyInfo> parseFiles(const std::vector<std::string>&);
};

class Liberty {
    std::vector<LibertyInfo> infos{};
    std::unordered_set<std::string_view> ff_types{};

   public:
    Liberty() = default;
    Liberty(const std::vector<std::string>&);
    bool isFF(std::string_view) const;
};
