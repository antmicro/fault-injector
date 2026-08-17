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
#include "FaultStrategyRunner.h"
#include "LogUtils.h"
#include "Signal.h"
#include "Utils.h"

#include <algorithm>
#include <cassert>

BendelStrategy::BendelStrategy(const Config& config, const BendelConfig& bendelConfig)
    : FaultStrategy(config), bendel_config(bendelConfig) {}

double BendelStrategy::eventTime(
    const Signal& signal,
    const BendelConfig::Stream& stream,
    FaultStrategy::RandomGen& gen
) {
    thread_local std::exponential_distribution<double> dist;

    const double E = stream.energy;
    const double Y = std::sqrt(18.0 / stream.A) * (E * 1e-6 - stream.A);
    const double X =
        std::pow(stream.B / stream.A, 14.0) * std::pow((1.0 - std::exp(-0.18 * std::sqrt(Y))), 4.0);
    const double cos = std::cos(seu::deg2rad(seu::FLUX_THETA));
    const double h = X * stream.flux_phi * signal.area * cos;
    assert(E >= 0 && h != 0);
    return dist(gen.random_generator, std::exponential_distribution<double>::param_type{h});
}

std::vector<FaultEvent> BendelStrategy::generate(std::span<const Signal> signals) {
    LOG(INFO) << "Bendel strategy generating in parallel";

    auto eventTime =
        [&](const Signal& signal, const BendelConfig::Stream& stream, FaultStrategy::RandomGen& gen
        ) { return this->eventTime(signal, stream, gen); };
    auto maxTime = [&](const BendelConfig::Stream& stream) {
        return std::min(
            stream.fluence / stream.flux_phi, static_cast<double>(config.simulation_time)
        );
    };
    using BendelRunner =
        FaultStrategyRunner<BendelConfig::Stream, decltype(eventTime), decltype(maxTime)>;
    BendelRunner runner(eventTime, maxTime, config, bendel_config.streams, signals, this->gen);

    std::vector<FaultEvent> result = runner.generateInParallelByTimeSlice();
    LOG(INFO) << "Bendel strategy generated " << result.size() << " faults";
    return result;
}

std::shared_ptr<FaultStrategy> BendelStrategy::copy_with(FaultStrategy::Config new_config) {
    return std::make_shared<BendelStrategy>(new_config, this->bendel_config);
}
