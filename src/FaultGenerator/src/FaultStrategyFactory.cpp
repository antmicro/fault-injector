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

#include "FaultStrategyFactory.h"

#include "FaultStrategy.h"
#include "FaultStrategyBendel.h"
#include "FaultStrategyRandom.h"
#include "FaultStrategyWeibull.h"

#include <absl/log/check.h>
#include <absl/log/log.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WeibullConfig::Stream, let, flux_phi, max_time)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    WeibullConfig,
    streams,
    bit_count,
    cell_area,
    let_threshold,
    width,
    shape_parameter,
    limiting_cross_section
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    BendelConfig::Stream,
    name,
    A,
    B,
    energy,
    flux_phi,
    fluence
)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    BendelConfig,
    bit_count,
    device_area,
    num_cells,
    streams
)

std::shared_ptr<FaultStrategy> FaultStrategyFactory::buildFromJson(
    const FaultStrategy::Config& config,
    const nlohmann::json& model_config
) {
    std::string_view model_name = model_config.at("name").get<std::string_view>();
    if (model_name == "random") {
        // TODO Here maybe warn that random model received params, when it doesn't expect to
        LOG(INFO) << "Parsed random model from json";
        return std::make_shared<RandomStrategy>(config);
    } else if (model_name == "weibull") {
        const auto weibull_config = model_config.at("params").get<WeibullConfig>();
        LOG(INFO) << "Parsed weibull model from json";
        return std::make_shared<WeibullStrategy>(config, weibull_config);
    } else if (model_name == "bendel") {
        const auto bendel_config = model_config.at("params").get<BendelConfig>();
        LOG(INFO) << "Parsed bendel model from json";
        return std::make_shared<BendelStrategy>(config, bendel_config);
    } else {
        LOG(FATAL) << "Unknown model: " << model_name << "\n";
    }
}

std::shared_ptr<FaultStrategy> FaultStrategyFactory::defaultStrategy(
    const FaultStrategy::Config& config
) {
    LOG(INFO) << "Selecting default (random) model";
    return std::make_shared<RandomStrategy>(RandomStrategy{config});
}
