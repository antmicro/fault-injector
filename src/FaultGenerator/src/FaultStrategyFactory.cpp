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
#include "FaultStrategyRandom.h"
#include "FaultStrategyWeibull.h"

#include <iostream>
#include <memory>

void from_json(const nlohmann::json& json, WeibullConfig::Stream& config) {
    config.let = json.at("let").get<double>();
    config.flux_phi = json.at("flux_phi").get<double>();
    config.max_time = json.at("max_time").get<double>();
}

void from_json(const nlohmann::json& json, WeibullConfig& config) {
    config.cell_area = json.value("cell_area", config.cell_area);
    config.bit_count = json.value("bit_count", config.bit_count);

    config.streams = json.value<std::vector<WeibullConfig::Stream>>("streams", {});
}

std::shared_ptr<FaultStrategy> FaultStrategyFactory::build_from_json(
    const FaultStrategy::Config& config,
    const nlohmann::json& model_config) {
    std::string_view model_name = model_config.at("name").get<std::string_view>();
    if (model_name == "random") {
        // TODO Here maybe warn that random model received params, when it doesn't expect to
        return std::make_shared<RandomStrategy>(RandomStrategy(config));
    } else if (model_name == "weibull") {
        WeibullConfig weibullConfig = model_config.at("params").get<WeibullConfig>();
        return std::make_shared<WeibullStrategy>(WeibullStrategy{config, weibullConfig});
    } else {
        std::cerr << "Unknown model: " << model_name << "\n";
        std::exit(1);
    }
}

std::shared_ptr<FaultStrategy> FaultStrategyFactory::default_strategy(
    const FaultStrategy::Config& config) {
    return std::make_shared<RandomStrategy>(RandomStrategy{config});
}
