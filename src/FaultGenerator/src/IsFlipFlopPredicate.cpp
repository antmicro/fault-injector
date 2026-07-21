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

#include "IsFlipFlopPredicate.h"

#include "Cell.h"
#include "Liberty.h"
#include "LogUtils.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

static const std::unordered_set<std::string_view> ff_name_suffixes = {
    // Source:
    // https://yosyshq.readthedocs.io/projects/yosys/en/stable/cell/word_reg.html
    "$dff",
    "$adff",
    "$aldff",
    "$dffsr",
    "$sdff",
    "$dffe",
    "$adffe",
    "$aldffe",
    "$dffsre",
    "$sdffe",
    "$sdffce"
};

class ByCommonNameSuffixes {
    static constexpr char separator = '$';

   public:
    bool operator()(const Cell& cell, const Liberty& liberty) {
        std::string_view cell_ff_type =
            std::string_view(cell.name).substr(cell.name.find_last_of(separator));
        bool result = ff_name_suffixes.contains(cell_ff_type);
        VLOG(1) << "Trying ByCommonNameSuffixes predicate, for: " << cell << " with: " << result;
        return result;
    }
};

static const std::vector<std::string_view> ff_type_prefixes = {
    // Source:
    // https://yosyshq.readthedocs.io/projects/yosys/en/0.40/yosys_internals/formats/cell_library.html
    "$_DFF",
    "$_SDFF"
};

class ByCommonTypePrefix {
   public:
    bool operator()(const Cell& cell, const Liberty& liberty) {
        bool result =
            std::find_if(ff_type_prefixes.begin(), ff_type_prefixes.end(), [&cell](auto prefix) {
                return cell.type.starts_with(prefix);
            }) != ff_type_prefixes.end();
        VLOG(1) << "Trying ByCommonTypePrefix predicate, for: " << cell << " with: " << result;
        return result;
    }
};

class ByKnownFFTypes {
   public:
    bool operator()(const Cell& cell, const Liberty& liberty) const {
        auto result = liberty.isFF(cell.type);
        LOG(INFO) << "Trying ByKnownFFTypes predicate, for: " << cell << " with: " << result;
        return result;
    }
};

/*****************************************************************************/

namespace {
const std::vector<IsFlipFlop::PredType> initialized_predicates{
    ByCommonNameSuffixes(),
    ByCommonTypePrefix(),
    ByKnownFFTypes(),
};
}  // namespace

bool IsFlipFlop::check(const Cell& cell, const Liberty& liberty) {
    auto it = std::ranges::find_if(initialized_predicates, [&cell, &liberty](const auto& pred) {
        return pred(cell, liberty);
    });
    return it != initialized_predicates.end();
}
