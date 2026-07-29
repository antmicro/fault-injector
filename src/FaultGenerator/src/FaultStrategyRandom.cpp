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

#include "FaultStrategyRandom.h"

#include "Constants.h"
#include "FaultEvent.h"
#include "FaultStrategy.h"

#include <algorithm>
#include <random>

RandomStrategy::RandomStrategy(const Config& config) : FaultStrategy(config) {}

std::vector<FaultEvent> RandomStrategy::generate(std::span<const Signal> signals) {
    std::uniform_int_distribution<uint64_t> time_dist(0, config.simulation_time);
    std::vector<uint64_t> time_values;
    time_values.reserve(config.num_of_events);
    for (unsigned int index = 0; index < config.num_of_events; ++index) {
        time_values.push_back(time_dist(gen.random_generator));
    }
    std::sort(time_values.begin(), time_values.end());

    std::uniform_int_distribution<uint32_t> bit_dist(0, seu::MAX_BITS);
    std::uniform_int_distribution<uint32_t> sig_dist(0, signals.size() - 1);
    std::vector<FaultEvent> fault_events;
    fault_events.reserve(config.num_of_events);
    for (unsigned int index = 0; index < config.num_of_events; ++index) {
        auto idx = sig_dist(gen.random_generator);
        const Signal& signal = signals[idx];

        fault_events.emplace_back(
            signals.cbegin() + idx,
            time_values[index],
            signal.signal_path,
            bit_dist(gen.random_generator) % signal.width,
            faultEventType(signal.type)
        );
    }
    return fault_events;
}

std::shared_ptr<FaultStrategy> RandomStrategy::copy_with(FaultStrategy::Config new_config) {
    RandomStrategy oth = RandomStrategy(new_config);
    return std::make_shared<RandomStrategy>(oth);
}
