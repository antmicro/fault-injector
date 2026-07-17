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
#include "Signal.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

struct Cell {
    const double area;
};

double WeibullStrategy::eventTime(
    const Cell& cell,
    const WeibullConfig::Stream& stream,
    double g0
) {
    const double cos = std::cos(seu::deg2rad(seu::FLUX_THETA));
    const double L0 = weibull_config.let_threshold;
    const double W = weibull_config.width;
    const double s = weibull_config.shape_parameter;

    const double g = g0 * (1.0 - std::exp(-std::pow((stream.let / cos - L0) / W, s)));
    const double h = g * stream.flux_phi * cell.area * cos;
    const double p = real_dist(gen.random_generator);
    const double dt = -std::log(1 - p) / h;
    return dt;
}

struct History {
    double curr_time = 0;
    int stream_id = 0;
};

WeibullStrategy::WeibullStrategy(const Config& config, const WeibullConfig& weibullConfig)
    : FaultStrategy(config), weibull_config(weibullConfig) {}

std::vector<FaultEvent> WeibullStrategy::generate(std::span<const Signal> signals) {
    std::vector<FaultEvent> result;
    for (size_t i = 0; i < weibull_config.streams.size(); ++i) {
        std::vector<FaultEvent> current = generate(weibull_config.streams[i], signals);
        std::printf("#%ld: %ld\n", i, current.size());
        for (auto& r : current) {
            result.emplace_back(r);
        }
    }
    return result;
}

std::vector<FaultEvent> WeibullStrategy::generate(
    const WeibullConfig::Stream& stream_data,
    std::span<const Signal> signals
) {
    std::vector<FaultEvent> result;
    Cell cell{.area = weibull_config.cell_area};
    const double g0 = weibull_config.limiting_cross_section *
                      (static_cast<double>(weibull_config.bit_count) / signals.size());

    std::vector<WeibullConfig::Stream> streams = {stream_data};
    std::vector<std::vector<double>> event_time(
        signals.size(), std::vector<double>{eventTime(cell, streams.front(), g0)}
    );
    std::vector<std::vector<History>> history(signals.size());
    double curr_time = 0.0;
    std::uint64_t total_hits = 0;

    while (true) {
        std::vector<double> time_list(event_time.size());
        std::vector<int> indx_list(event_time.size());

        for (size_t i = 0; i < event_time.size(); ++i) {
            // TODO use priority_queue
            auto min_it = std::min_element(event_time[i].begin(), event_time[i].end());

            time_list[i] = *min_it;
            indx_list[i] = static_cast<int>(std::distance(event_time[i].begin(), min_it));
        }

        // TODO use priority_queue
        auto min_cell_it = std::min_element(time_list.begin(), time_list.end());

        int cell_id = static_cast<int>(std::distance(time_list.begin(), min_cell_it));

        int stream_id = indx_list[cell_id];

        curr_time = event_time[cell_id][stream_id];

        // Finish
        if (curr_time >= stream_data.max_time) {
            break;
        }

        // Store event history
        history[cell_id].emplace_back(History{curr_time, stream_id});
        const auto& signal = signals[cell_id];
        result.emplace_back(FaultEvent{
            static_cast<std::uint64_t>(curr_time),
            signal.signal_path,
            bit_dist(gen.random_generator) % signal.num_of_bits,
            FaultEventType::SINGLE_EVENT_UPSET
        });
        ++total_hits;

        // Schedule next one
        event_time[cell_id][stream_id] += eventTime(cell, streams[stream_id], g0);
    }

    return result;
}

std::shared_ptr<FaultStrategy> WeibullStrategy::copy_with(FaultStrategy::Config new_config) {
    WeibullStrategy oth = WeibullStrategy(new_config, this->weibull_config);
    return std::make_shared<WeibullStrategy>(oth);
}
