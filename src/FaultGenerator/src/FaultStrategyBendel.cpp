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

#include "Constants.h"
#include "FaultEvent.h"
#include "LogUtils.h"
#include "ScheduledEvent.h"
#include "Signal.h"
#include "Utils.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <queue>

BendelStrategy::BendelStrategy(const Config& config, const BendelConfig& bendelConfig)
    : FaultStrategy(config), bendel_config(bendelConfig) {}

double BendelStrategy::eventTime(const Signal& signal, const BendelConfig::Stream& stream) {
    const double E = stream.energy;
    const double Y = std::sqrt(18.0 / stream.A) * (E * 1e-6 - stream.A);
    const double X =
        std::pow(stream.B / stream.A, 14.0) * std::pow((1.0 - std::exp(-0.18 * std::sqrt(Y))), 4.0);
    const double cos = std::cos(seu::deg2rad(seu::FLUX_THETA));
    const double h = X * stream.flux_phi * signal.area * cos;
    const double p = real_dist(gen.random_generator);
    assert(E >= 0 && h != 0);
    const double dt = -std::log(1.0 - p) / h;
    return dt;
}

std::vector<FaultEvent> BendelStrategy::generate(std::span<const Signal> signals) {
    std::vector<FaultEvent> result;
    for (size_t i = 0; i < bendel_config.streams.size(); ++i) {
        // TODO parallel generation of events for every stream independently
        LOG(INFO) << "Starting generating for stream #" << i;
        std::vector<FaultEvent> current = generate(bendel_config.streams[i], signals);
        LOG(INFO) << "Stream #" << i << " generated " << current.size() << " faults\n";
        const auto result_size = result.size();
        std::copy(current.begin(), current.end(), std::back_inserter(result));
        std::inplace_merge(result.begin(), result.begin() + result_size, result.end());
    }
    LOG(INFO) << "Bendel strategy generated " << result.size() << " faults";
    return result;
}

std::vector<FaultEvent> BendelStrategy::generate(
    const BendelConfig::Stream& stream,
    std::span<const Signal> signals
) {
    std::vector<FaultEvent> result;

    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>>
        event_queue;

    for (std::size_t signal_id = 0; signal_id < signals.size(); ++signal_id) {
        event_queue.push(ScheduledEvent{
            .time = eventTime(signals[signal_id], stream),
            .signal_id = signal_id,
        });
    }

    const double max_time =
        std::min(stream.fluence / stream.flux_phi, static_cast<double>(config.simulation_time));
    while (!event_queue.empty()) {
        const ScheduledEvent next = event_queue.top();
        event_queue.pop();

        if (next.time >= max_time) {
            break;
        }
        if (config.tooManyEventsGenerated(result.size())) {
            LOG(WARNING) << "Amount of generated fault exceeded number specified in the config. "
                            "Stoping generation.";
            break;
        }

        const auto& signal = signals[next.signal_id];
        result.emplace_back(
            signals.cbegin() + next.signal_id,
            static_cast<std::uint64_t>(next.time),
            signal.signal_path,
            bit_dist(gen.random_generator) % signal.width,
            faultEventType(signal.type)
        );

        // Schedule next one
        event_queue.emplace(next.time + eventTime(signal, stream), next.signal_id);
    }

    return result;
}

std::shared_ptr<FaultStrategy> BendelStrategy::copy_with(FaultStrategy::Config new_config) {
    BendelStrategy oth = BendelStrategy(new_config, this->bendel_config);
    return std::make_shared<BendelStrategy>(oth);
}
