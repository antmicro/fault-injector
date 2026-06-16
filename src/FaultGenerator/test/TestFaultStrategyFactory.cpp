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
#include "FaultStrategyWeibull.h"
#include "GlobalOpts.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace nlohmann::literals;  // Required for the _json literal

void from_json(const nlohmann::json&, GlobalOpts&);

TEST(RandomStrategyTests, ExampleTest) {
    auto random_strategy_json =
        R"json({
  "model": {
    "name": "random",
    "params" : {}
  },
  "params": {
    "num_of_events": 101,
    "seed": 13,
    "simulation_time": 102,
    "sig_path_prefix": "top",
    "top_module": "dff_worker",
    "top_instance": "worker",
    "netlist_path": "worker.json",
    "fault_campaign_out": "random_file.csv"
  }
})json"_json;

    GlobalOpts actual = random_strategy_json.get<GlobalOpts>();
    auto random = std::dynamic_pointer_cast<RandomStrategy>(actual.strategy);

    EXPECT_TRUE(random);
    EXPECT_EQ(random->config.num_of_events, 101);
    EXPECT_EQ(random->config.seed, 13);
    EXPECT_EQ(random->config.simulation_time, 102);

    EXPECT_EQ(actual.sig_path_prefix, "top");
    EXPECT_EQ(actual.top_module, "dff_worker");
    EXPECT_EQ(actual.top_instance, "worker");
    EXPECT_EQ(actual.netlist_path, "worker.json");
    EXPECT_EQ(actual.fault_campaign_out, "random_file.csv");
}

TEST(WeibullStrategyTests, ExampleTest) {
    auto weibull_strategy_json =
        R"json({
  "model": {
    "name": "weibull",
    "params" : {
      "cell_area": 0.25e-6,
      "streams": [{
        "let": 111.0,
        "flux_phi": 123.0,
        "max_time": 321.0
      }]
    }
  },
  "params": {
    "num_of_events": 103,
    "seed": 56,
    "simulation_time": 104,
    "sig_path_prefix": "top",
    "top_module": "dff_worker",
    "top_instance": "worker",
    "netlist_path": "worker.json",
    "fault_campaign_out": "random_file.csv"
  }
})json"_json;

    GlobalOpts actual = weibull_strategy_json.get<GlobalOpts>();
    auto weibull = std::dynamic_pointer_cast<WeibullStrategy>(actual.strategy);

    EXPECT_TRUE(weibull);
    EXPECT_EQ(weibull->config.num_of_events, 103);
    EXPECT_EQ(weibull->config.seed, 56);
    EXPECT_EQ(weibull->config.simulation_time, 104);

    auto wconfig = weibull->weibullConfig;
    EXPECT_EQ(wconfig.streams.size(), 1);
    EXPECT_DOUBLE_EQ(wconfig.streams[0].let, 111.0);
    EXPECT_DOUBLE_EQ(wconfig.streams[0].flux_phi, 123.0);
    EXPECT_DOUBLE_EQ(wconfig.streams[0].max_time, 321.0);

    EXPECT_EQ(actual.sig_path_prefix, "top");
    EXPECT_EQ(actual.top_module, "dff_worker");
    EXPECT_EQ(actual.top_instance, "worker");
    EXPECT_EQ(actual.netlist_path, "worker.json");
    EXPECT_EQ(actual.fault_campaign_out, "random_file.csv");
}
