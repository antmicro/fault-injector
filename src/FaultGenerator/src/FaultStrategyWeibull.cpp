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

#include "FaultStrategy.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

struct Cell {
    const double area;
    struct Weibull {
        double L0 = 1.09 * 1e5;  // [Mev * cm^2 / mg]
        double W = 39.25 * 1e5;  // [Mev * cm^2 / mg]
        double s = 1.116;
        double g0 = 0.284 * 1e-4;  // [s * cm ^ 2 / bit]
    };
    Weibull weibull;
};

struct Stream {
    const double flux_phi;
    const double let;
    double flux_theta = 0.0;
    static constexpr double deg2rad(double deg) { return deg * std::numbers::pi / 180.0; }
    double eventTime(const Cell& cell,
                     WeibullStrategy::RandomGen& gen,
                     std::uniform_real_distribution<double>& dist) {
        const double cos = std::cos(deg2rad(flux_theta));

        const double g = cell.weibull.g0 *
                         (1.0 - std::exp(-std::pow((let / cos - cell.weibull.L0) / cell.weibull.W,
                                                   cell.weibull.s)));
        const double h = g * flux_phi * cell.area * cos;
        const double p = dist(gen.random_generator);
        const double dt = -std::log(1 - p) / h;
        return dt;
    }
};

struct History {
    double curr_time = 0;
    int stream_id = 0;
};

WeibullStrategy::WeibullStrategy(const Config& config, const WeibullConfig& weibullConfig)
    : FaultStrategy(config), weibullConfig(weibullConfig) {}

std::vector<FaultEvent> WeibullStrategy::generate(const std::span<Signal>& signals) {
    std::vector<FaultEvent> result;
    for (size_t i = 0; i < weibullConfig.streams.size(); ++i) {
        std::vector<FaultEvent> current = generate(weibullConfig.streams[i], signals);
        std::printf("#%ld: %ld\n", i, current.size());
        for (auto& r : current) {
            result.emplace_back(r);
        }
    }
    return result;
}

std::vector<FaultEvent> WeibullStrategy::generate(const WeibullConfig::Stream& streamData,
                                                  const std::span<Signal>& signals) {
    std::vector<FaultEvent> result;
    // TODO support cells with different areas
    Cell cell{.area = weibullConfig.cell_area};
    cell.weibull.g0 *= static_cast<double>(weibullConfig.bit_count) / signals.size();

    std::vector<Stream> streams = {Stream{.flux_phi = streamData.flux_phi, .let = streamData.let}};
    std::vector<std::vector<double>> event_time(
        signals.size(), std::vector<double>{streams.front().eventTime(cell, gen, real_dist)});
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

        double prev_time = curr_time;
        curr_time = event_time[cell_id][stream_id];

        // Finish
        if (curr_time >= streamData.max_time) {
            break;
        }

        // Store event history
        history[cell_id].emplace_back(History{curr_time, stream_id});
        const auto& signal = signals[cell_id];
        result.emplace_back(FaultEvent{static_cast<std::uint64_t>(curr_time), signal.signal_path,
                                       bit_dist(gen.random_generator) % signal.num_of_bits,
                                       FaultEventType::SINGLE_EVENT_UPSET});
        ++total_hits;

        // Schedule next one
        event_time[cell_id][stream_id] += streams[stream_id].eventTime(cell, gen, real_dist);
    }

    return result;
}
