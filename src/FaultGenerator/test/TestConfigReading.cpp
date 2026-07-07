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

    EXPECT_EQ(actual.sig_path_prefix, "top");
    EXPECT_EQ(actual.top_module, "dff_worker");
    EXPECT_EQ(actual.top_instance, "worker");
    EXPECT_EQ(actual.netlist_path, "worker.json");
    EXPECT_EQ(actual.fault_campaign_out, "random_file.csv");

    ASSERT_TRUE(random);
    EXPECT_EQ(random->config.num_of_events, 101);
    EXPECT_EQ(random->config.seed, 13);
    EXPECT_EQ(random->config.simulation_time, 102);
}

TEST(WeibullStrategyTests, ExampleTest) {
    auto weibull_strategy_json =
        R"json({
  "model": {
    "name": "weibull",
    "params" : {
      "cell_area": 0.25e-23,
      "bit_count": 140000,
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

    EXPECT_EQ(actual.sig_path_prefix, "top");
    EXPECT_EQ(actual.top_module, "dff_worker");
    EXPECT_EQ(actual.top_instance, "worker");
    EXPECT_EQ(actual.netlist_path, "worker.json");
    EXPECT_EQ(actual.fault_campaign_out, "random_file.csv");

    auto weibull = std::dynamic_pointer_cast<WeibullStrategy>(actual.strategy);
    ASSERT_TRUE(weibull);
    EXPECT_EQ(weibull->config.num_of_events, 103);
    EXPECT_EQ(weibull->config.seed, 56);
    EXPECT_EQ(weibull->config.simulation_time, 104);

    auto wconfig = weibull->weibullConfig;
    EXPECT_DOUBLE_EQ(wconfig.cell_area, 0.25e-23);
    EXPECT_DOUBLE_EQ(wconfig.bit_count, 140000);
    ASSERT_EQ(wconfig.streams.size(), 1);
    EXPECT_DOUBLE_EQ(wconfig.streams[0].let, 111.0);
    EXPECT_DOUBLE_EQ(wconfig.streams[0].flux_phi, 123.0);
    EXPECT_DOUBLE_EQ(wconfig.streams[0].max_time, 321.0);
}

TEST(LibertyFilesTests, JsonConfigTest) {
    auto json =
        R"json({
  "model": {
    "name": "random",
    "params" : {}
  },
  "params": {
    "liberty_paths": [
        "file1.lib",
        "file2.lib",
        "file3.lib"
    ]
  }
})json"_json;

    GlobalOpts actual = json.get<GlobalOpts>();
    auto& liberty_files = actual.liberty_paths;

    EXPECT_EQ(liberty_files.size(), 3);
    EXPECT_EQ(liberty_files[0], "file1.lib");
    EXPECT_EQ(liberty_files[1], "file2.lib");
    EXPECT_EQ(liberty_files[2], "file3.lib");
}

TEST(LibertyFilesTests, NoArgsTest) {
    const int argc = 1;
    const char* argv[argc] = {"test_bin"};
    GlobalOpts actual = GlobalOpts::parse_cmd_args(argc, (char**)argv);
    auto& liberty_files = actual.liberty_paths;

    EXPECT_EQ(liberty_files.size(), 0);
}

TEST(LibertyFilesTests, ArgsTestWithEq) {
    const int argc = 2;
    const char* argv[argc] = {
        "test_bin",
        "--liberty_paths=file1.lib,file2.lib,file3.lib",
    };
    GlobalOpts actual = GlobalOpts::parse_cmd_args(argc, (char**)argv);
    auto& liberty_files = actual.liberty_paths;

    ASSERT_EQ(liberty_files.size(), 3);
    EXPECT_EQ(liberty_files[0], "file1.lib");
    EXPECT_EQ(liberty_files[1], "file2.lib");
    EXPECT_EQ(liberty_files[2], "file3.lib");
}

TEST(LibertyFilesTests, ArgsTestWithoutEq) {
    const int argc = 3;
    const char* argv[argc] = {
        "test_bin",
        "--liberty_paths",
        "file1.lib,file2.lib,file3.lib",
    };
    GlobalOpts actual = GlobalOpts::parse_cmd_args(argc, (char**)argv);
    auto& liberty_files = actual.liberty_paths;

    ASSERT_EQ(liberty_files.size(), 3);
    EXPECT_EQ(liberty_files[0], "file1.lib");
    EXPECT_EQ(liberty_files[1], "file2.lib");
    EXPECT_EQ(liberty_files[2], "file3.lib");
}
