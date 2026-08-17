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
#include "FaultStrategyRunner.h"
#include "LogUtils.h"
#include "Signal.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <vector>

WeibullStrategy::WeibullStrategy(const Config& config, const WeibullConfig& weibullConfig)
    : FaultStrategy(config), weibull_config(weibullConfig) {}

// cannot be constexpr, as std::cos is constexpr only in c++26
const double cos_theta = std::cos(seu::deg2rad(seu::FLUX_THETA));
double WeibullStrategy::eventTime(
    const Signal& signal,
    const WeibullConfig::Stream& stream,
    double g0,
    FaultStrategy::RandomGen& gen
) {
    thread_local std::exponential_distribution<double> dist;

    const double L0 = weibull_config.let_threshold;
    const double W = weibull_config.width;
    const double s = weibull_config.shape_parameter;

    const double g = g0 * (1.0 - std::exp(-std::pow((stream.let / cos_theta - L0) / W, s)));
    const double h = g * stream.flux_phi * signal.area * cos_theta;
    assert(h != 0);
    return dist(gen.random_generator, std::exponential_distribution<double>::param_type{h});
}

std::vector<FaultEvent> WeibullStrategy::generate(std::span<const Signal> signals) {
    LOG(INFO) << "Weibull strategy generating in parallel";

    const double g0 = weibull_config.limiting_cross_section *
                      (static_cast<double>(weibull_config.bit_count) / signals.size());
    auto eventTime =
        [&](const Signal& signal, const WeibullConfig::Stream& stream, FaultStrategy::RandomGen& gen
        ) { return this->eventTime(signal, stream, g0, gen); };
    auto maxTime = [&](const WeibullConfig::Stream& stream) {
        return std::min(stream.max_time, static_cast<double>(config.simulation_time));
    };
    using WeibullRunner =
        FaultStrategyRunner<WeibullConfig::Stream, decltype(eventTime), decltype(maxTime)>;
    WeibullRunner runner(eventTime, maxTime, config, weibull_config.streams, signals, this->gen);

    std::vector<FaultEvent> result = runner.generateInParallelByTimeSlice();
    LOG(INFO) << "Weibull strategy generated " << result.size() << " faults";
    return result;
}

std::shared_ptr<FaultStrategy> WeibullStrategy::copy_with(FaultStrategy::Config new_config) {
    return std::make_shared<WeibullStrategy>(new_config, weibull_config);
}
