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

#include "FaultStrategyWeibull.h"

#include "FaultEvent.h"
#include "FaultStrategy.h"
#include "LogUtils.h"
#include "ScheduledEvent.h"
#include "Signal.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <random>
#include <vector>

WeibullStrategy::WeibullStrategy(const Config& config, const WeibullConfig& weibullConfig)
    : FaultStrategy(config), weibull_config(weibullConfig) {}

double WeibullStrategy::cellArea(const Signal& signal) {
    if (!signal.area) {
        // default cell area in case cell wasn't defined in liberty file
        return weibull_config.cell_area;
    }
    return *signal.area;
}

double WeibullStrategy::eventTime(
    const Signal& signal,
    const WeibullConfig::Stream& stream,
    double g0
) {
    const double cos = std::cos(seu::deg2rad(seu::FLUX_THETA));
    const double L0 = weibull_config.let_threshold;
    const double W = weibull_config.width;
    const double s = weibull_config.shape_parameter;

    const double g = g0 * (1.0 - std::exp(-std::pow((stream.let / cos - L0) / W, s)));
    const double h = g * stream.flux_phi * cellArea(signal) * cos;
    const double p = real_dist(gen.random_generator);
    SEE_INTERNAL_CHECK(h != 0) << "Assertion in WeibullStrategy::eventTime failed";
    const double dt = -std::log(1 - p) / h;
    return dt;
}

std::vector<FaultEvent> WeibullStrategy::generate(std::span<const Signal> signals) {
    std::vector<FaultEvent> result;
    for (size_t i = 0; i < weibull_config.streams.size(); ++i) {
        // TODO parallel generation of events for every stream independently
        LOG(INFO) << "Starting generating for stream #" << i;
        std::vector<FaultEvent> current = generate(weibull_config.streams[i], signals);
        LOG(INFO) << "Stream #" << i << " generated " << current.size() << " faults\n";
        const auto result_size = result.size();
        std::copy(current.begin(), current.end(), std::back_inserter(result));
        std::inplace_merge(result.begin(), result.begin() + result_size, result.end());
    }
    return result;
}

std::vector<FaultEvent> WeibullStrategy::generate(
    const WeibullConfig::Stream& stream_data,
    std::span<const Signal> signals
) {
    std::vector<FaultEvent> result;
    const double g0 = weibull_config.limiting_cross_section *
                      (static_cast<double>(weibull_config.bit_count) / signals.size());

    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>>
        event_queue;

    for (std::size_t signal_id = 0; signal_id < signals.size(); ++signal_id) {
        event_queue.push(ScheduledEvent{
            .time = eventTime(signals[signal_id], stream_data, g0),
            .signal_id = signal_id,
        });
    }

    while (!event_queue.empty()) {
        const ScheduledEvent next = event_queue.top();
        event_queue.pop();

        // Finish
        if (next.time >= stream_data.max_time) {
            break;
        }

        const auto& signal = signals[next.signal_id];
        result.emplace_back(FaultEvent{
            static_cast<std::uint64_t>(next.time),
            signal.signal_path,
            bit_dist(gen.random_generator) % signal.num_of_bits,
            FaultEventType::SINGLE_EVENT_UPSET
        });

        // Schedule next one
        event_queue.emplace(next.time + eventTime(signal, stream_data, g0), next.signal_id);
    }

    return result;
}

std::shared_ptr<FaultStrategy> WeibullStrategy::copy_with(FaultStrategy::Config new_config) {
    WeibullStrategy oth = WeibullStrategy(new_config, this->weibull_config);
    return std::make_shared<WeibullStrategy>(oth);
}
