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

#include "SignalGroupingFaultFormatter.h"

#include "FaultEvent.h"
#include "LogUtils.h"
#include "Signal.h"

#include <cassert>
#include <charconv>

namespace {
struct BracketIndices {
    std::size_t bopen_pos;
    std::size_t bclose_pos;
};

std::optional<BracketIndices> findNotEscapedBrackets(std::string_view signal_path) {
    auto bclose_pos = signal_path.size() - 1;
    if (signal_path.size() == 0 || signal_path[bclose_pos] != ']') {
        // path doesn't end with closing brackets, indexing not present
        return std::nullopt;
    }
    if (bclose_pos > 0 && signal_path[bclose_pos] == '\\') {
        // path ends with escaped brackets, indexing not present
        return std::nullopt;
    }
    auto bopen_pos = signal_path.find_last_of('[', bclose_pos);
    if (bopen_pos == signal_path.npos || bopen_pos == 0) {
        // didn't find opening bracket, or encountered an empty signal, indexing not present
        return std::nullopt;
    }
    return BracketIndices{bopen_pos, bclose_pos};
}
};  // namespace

SignalGroupingFaultFormatter::SignalGroupingFaultFormatter(std::span<const Signal> signals)
    : real_signals_cache(signals.size()), signals(signals) {
    for (std::size_t i = 0; i < signals.size(); i++) {
        insert(i, signals[i]);
    }
}

void SignalGroupingFaultFormatter::insertUngrouped(std::size_t id, const Signal& signal) {
    real_signals_cache[id] = SignalData{.path = signal.signal_path, .bit_idx = 0};
}

void SignalGroupingFaultFormatter::insert(std::size_t id, const Signal& signal) {
    std::string_view signal_path{signal.signal_path};
    if (signal.width > 1) {
        VLOG(2) << "[" << signal.signal_path << "] signal.width>1. Abandoning grouping.";
        insertUngrouped(id, signal);
        return;
    }

    auto idxs = findNotEscapedBrackets(signal_path);
    if (!idxs) {
        VLOG(2) << "[" << signal.signal_path << "] is not indexed. Abandoning grouping.";
        insertUngrouped(id, signal);
        return;
    }
    auto [bopen_pos, bclose_pos] = *idxs;

    // When presence of indexing was detected, this call strips the indexing:
    // "top.worker.resp[6]" -> "top.worker.resp"
    std::string_view real_signal_name = signal_path.substr(0, bopen_pos);
    std::string_view idx_str = signal_path.substr(bopen_pos + 1, bclose_pos - bopen_pos - 1);
    std::size_t idx;
    const auto [ptr, ec] = std::from_chars(idx_str.data(), idx_str.data() + idx_str.size(), idx);
    if (ec != std::errc{} || ptr != idx_str.data() + idx_str.size()) {
        VLOG(2) << "[" << signal.signal_path
                << "] is not indexed with a number. Abandoning grouping.";
        insertUngrouped(id, signal);
        return;
    }

    VLOG(2) << "[" << signal.signal_path << "] grouped correctly as {" << real_signal_name << ", "
            << idx << "}";
    real_signals_cache[id] = SignalData{.path = real_signal_name, .bit_idx = idx};
}

FaultEvent SignalGroupingFaultFormatter::operator()(FaultEvent event) const {
    std::size_t id = event.it - signals.cbegin();
    auto& [path, bit_idx] = real_signals_cache[id];
    if (event.bit_index == 0) {
        event.signal_path = path;
        event.bit_index = bit_idx;
    }
    return event;
}
