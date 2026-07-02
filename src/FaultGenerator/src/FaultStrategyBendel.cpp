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

#include "FaultStrategyBendel.h"
#include <algorithm>
#include <cassert>
#include <utility>
#include "Constants.h"
#include "Utils.h"

BendelStrategy::BendelStrategy(const Config& config, const BendelConfig& bendelConfig)
    : FaultStrategy(config), bendel_config(bendelConfig) {}

double BendelStrategy::eventTime(const Cell& cell, const BendelConfig::Stream& stream) {
    const double E = stream.energy;
    const double Y = std::sqrt(18.0 / stream.A) * (E * 1e-6 - stream.A);
    const double X = 1e-6 * std::pow(stream.B / stream.A, 14.0) *
                     std::pow((1.0 - std::exp(-0.18 * std::sqrt(Y))), 4.0);
    const double cos = std::cos(seu::deg2rad(seu::FLUX_THETA));
    const double h = X * stream.flux_phi * cell.area * cos;
    const double p = real_dist(gen.random_generator);
    assert(E >= 0 && h != 0);
    const double dt = -std::log(1.0 - p) / h;
    return dt;
}

std::vector<FaultEvent> BendelStrategy::generate(std::span<const Signal> signals) {
    std::vector<FaultEvent> result;
    for (size_t i = 0; i < bendel_config.streams.size(); ++i) {
        // TODO parallel generation of events for every stream independently
        std::vector<FaultEvent> current = generate(bendel_config.streams[i], signals);
        std::printf("#%ld: %ld\n", i, current.size());
        const auto result_size = result.size();
        result.insert(result.end(), current.begin(), current.end());
        std::inplace_merge(result.begin(), result.begin() + result_size, result.end());
    }
    return result;
}

std::vector<FaultEvent> BendelStrategy::generate(const BendelConfig::Stream& stream,
                                                 std::span<const Signal> signals) {
    std::vector<FaultEvent> result;
    const Cell cell{.area = bendel_config.device_area / bendel_config.num_cells};

    const double max_time = stream.fluence / stream.flux_phi;
    std::vector<double> event_time(signals.size(), eventTime(cell, stream));
    double curr_time = 0.0;

    while (true) {
        // TODO use priority_queue
        const auto min_cell_it = std::min_element(event_time.begin(), event_time.end());
        const size_t cell_id = std::distance(event_time.begin(), min_cell_it);

        curr_time = event_time[cell_id];

        if (curr_time >= max_time) {
            break;
        }

        const auto& signal = signals[cell_id];
        result.emplace_back(static_cast<std::uint64_t>(curr_time), signal.signal_path,
                            bit_dist(gen.random_generator) % signal.num_of_bits,
                            FaultEventType::SINGLE_EVENT_UPSET);

        // Schedule next one
        event_time[cell_id] += eventTime(cell, stream);
    }

    return result;
}

std::shared_ptr<FaultStrategy> BendelStrategy::copy_with(FaultStrategy::Config new_config) {
    BendelStrategy oth = BendelStrategy(new_config, this->bendel_config);
    return std::make_shared<BendelStrategy>(oth);
}
