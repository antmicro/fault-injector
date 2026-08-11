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

#include "IsFlipFlopPredicate.h"
#include "Liberty.h"
#include "PlacementInfo.h"
#include "Signal.h"
#include "SignalCollector.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using namespace nlohmann::literals;  // Required for the _json literal

auto normal_json = R"json({
  "creator": "Yosys 0.33 (git sha1 2584903a060)",
  "modules": {
    "dff_worker": {
      "cells": {
        "$add$/path/to/file/worker/dff_worker.v:33$2": {
          "type": "$add",
          "parameters": {
            "A_SIGNED": "00000000000000000000000000000000",
            "A_WIDTH": "00000000000000000000000000100000",
            "B_SIGNED": "00000000000000000000000000000000",
            "B_WIDTH": "00000000000000000000000000100000",
            "Y_WIDTH": "00000000000000000000000000100000"
          }
        },
        "$procmux$4": {
          "type": "$mux",
          "parameters": {
            "WIDTH": "00000000000000000000000000100000"
          }
        },
        "$procmux$7": {
          "type": "$mux",
          "parameters": {
            "WIDTH": "00000000000000000000000000100000"
          }
        },
        "counter$dff": {
          "type": "$dff",
          "parameters": {
            "CLK_POLARITY": "1",
            "WIDTH": "00000000000000000000000000100000"
          }
        },
        "resp$dff": {
          "type": "$dff",
          "parameters": {
            "CLK_POLARITY": "1",
            "WIDTH": "00000000000000000000000000100000"
          }
        }
      }
    }
  }
})json"_json;
std::string_view normal_top_module = "dff_worker";
std::string_view normal_top_instance = "worker";
std::string_view normal_sig_path_prefix = "top";
Liberty normal_liberty = {{LibertyInfo{
    "test",
    {{"$dff", {.area = 10.0, .ff_info = FlipFlopInfo{}}}},
}}};
const PlacementInfo normal_placement{
    std::nullopt,
    {
        CellPlacementInfo{
            "counter$dff",
            "$dff",
            {
                .width = 4,
                .height = 3,
                .x = 1,
                .y = 2,
            }
        },
        CellPlacementInfo{
            "resp$dff",
            "$dff",
            {
                .width = 5,
                .height = 6,
                .x = 8,
                .y = 7,
            }
        },
    }
};

TEST(SignalCollectorTests, EmptyNetlist) {
    auto empty_json = R"json({})json"_json;
    ASSERT_DEATH(
        {
            (void)SignalCollector("worker", "top", "", normal_liberty, normal_placement)
                .collectFromJSON(empty_json);
        },
        "Malformed netlist json"
    );
}

TEST(SignalCollectorTests, ModuleWithNoCells) {
    auto json_without_cells = R"json({
  "creator": "Yosys 0.33 (git sha1 2584903a060)",
  "modules": {
    "dff_worker": {
      "cells": {}
    }
  }
})json"_json;
    ASSERT_DEATH(
        {
            (void)SignalCollector(
                normal_top_module,
                normal_top_instance,
                normal_sig_path_prefix,
                normal_liberty,
                normal_placement
            )
                .collectFromJSON(json_without_cells);
        },
        "No signals found, cannot generate faults"
    );
}

TEST(SignalCollectorTests, EmptyTopModule) {
    const auto& json = normal_json;
    ASSERT_DEATH(
        {
            (void)SignalCollector(
                "", normal_top_instance, normal_sig_path_prefix, normal_liberty, normal_placement
            )
                .collectFromJSON(json);
        },
        "Top module not found. Cannot generate faults without signals"
    );
}

TEST(SignalCollectorTests, EmptyTopInstance) {
    const auto& json = normal_json;
    ASSERT_DEATH(
        {
            std::ignore =
                SignalCollector(
                    normal_top_module, "", normal_sig_path_prefix, normal_liberty, normal_placement
                )
                    .collectFromJSON(json);
        },
        "Empty top instance! Use --top_instance to specify it's name."
    );
}

TEST(SignalCollectorTests, EmptySigPathPrefix) {
    const auto& json = normal_json;
    const auto& signals =
        SignalCollector(
            normal_top_module, normal_top_instance, "", normal_liberty, normal_placement
        )
            .collectFromJSON(json);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 2);
    EXPECT_EQ(signals[0].path_prefix, "worker");
    EXPECT_EQ(signals[0].signal_name, "counter");
    EXPECT_EQ(signals[0].width, 32);
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
    EXPECT_EQ(signals[1].path_prefix, "worker");
    EXPECT_EQ(signals[1].signal_name, "resp");
    EXPECT_EQ(signals[1].width, 32);
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);
}

TEST(SignalCollectorTests, EmptyLiberty) {
    ASSERT_DEATH(
        {
            const auto& json = normal_json;
            const auto& signals = SignalCollector(
                                      normal_top_module,
                                      normal_top_instance,
                                      normal_sig_path_prefix,
                                      {},
                                      normal_placement
            )
                                      .collectFromJSON(json);
        },
        "No signals found, cannot generate faults"
    );
}

TEST(SignalCollectorTests, NormalNetlist) {
    const auto& json = normal_json;
    const auto& signals =
        SignalCollector(
            normal_top_module, normal_top_instance, normal_sig_path_prefix, normal_liberty, {}
        )
            .collectFromJSON(json);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 2);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].signal_name, "counter");
    EXPECT_EQ(signals[0].width, 32);
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
    EXPECT_EQ(signals[1].path_prefix, "top.worker");
    EXPECT_EQ(signals[1].signal_name, "resp");
    EXPECT_EQ(signals[1].width, 32);
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);
}

TEST(SignalCollectorTests, NormalNetlistWithPlacement) {
    const auto& json = normal_json;
    const auto& signals = SignalCollector(
                              normal_top_module,
                              normal_top_instance,
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement
    )
                              .collectFromJSON(json);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 2);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].signal_name, "counter");
    EXPECT_EQ(signals[0].width, 32);
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
    ASSERT_TRUE(signals[0].area);
    EXPECT_EQ(signals[0].area, 10.0);
    ASSERT_TRUE(signals[0].cell_placement);
    EXPECT_EQ(signals[0].cell_placement->width, 4);
    EXPECT_EQ(signals[0].cell_placement->height, 3);
    EXPECT_EQ(signals[0].cell_placement->x, 1);
    EXPECT_EQ(signals[0].cell_placement->y, 2);

    EXPECT_EQ(signals[1].path_prefix, "top.worker");
    EXPECT_EQ(signals[1].signal_name, "resp");
    EXPECT_EQ(signals[1].width, 32);
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);
    ASSERT_TRUE(signals[1].area);
    EXPECT_EQ(signals[1].area, 10.0);
    ASSERT_TRUE(signals[1].cell_placement);
    EXPECT_EQ(signals[1].cell_placement->width, 5);
    EXPECT_EQ(signals[1].cell_placement->height, 6);
    EXPECT_EQ(signals[1].cell_placement->x, 8);
    EXPECT_EQ(signals[1].cell_placement->y, 7);
}

auto json_with_hdlname = R"json({
  "modules": {
    "worker": {
      "cells": {
        "dff_worker0.counter[0]$_DFFE_PP_": {
          "type": "$_DFFE_PP_",
          "parameters": {
          },
          "attributes": {
            "hdlname": "dff_worker0 counter$dff"
          }
        },
        "dff_worker0.resp[0]$_DFFE_PP_": {
          "type": "$_DFFE_PP_",
          "parameters": {
          },
          "attributes": {
            "hdlname": "dff_worker0 resp$dff"
          }
        },
        "dff_worker0.resp[32:13]$_DFFE_PP_": {
          "type": "$_DFFE_PP_",
          "parameters": {
          },
          "attributes": {
            "hdlname": "dff_worker0 resp[32:13]$dff"
          }
        }
      }
    }
  }
}
)json"_json;
const std::string_view json_with_hdlname_top_module = "worker";
const std::string_view json_with_hdlname_top_instance = "worker";
const std::string_view json_with_hdlname_sig_path_prefix = "top";
const Liberty json_with_hdlname_liberty = {{LibertyInfo{
    "test",
    {{"$_DFFE_PP_", {.area = 10.0, .ff_info = FlipFlopInfo{}}}},
}}};
const PlacementInfo json_with_hdlname_placement{
    std::nullopt,
    {
        CellPlacementInfo{
            "dff_worker0.counter[0]$_DFFE_PP_",
            "$_DFFE_PP_",
            {
                .width = 4,
                .height = 3,
                .x = 1,
                .y = 2,
            }
        },
        CellPlacementInfo{
            "dff_worker0.resp[0]$_DFFE_PP_",
            "$_DFFE_PP_",
            {
                .width = 5,
                .height = 6,
                .x = 8,
                .y = 7,
            }
        },
    }
};

TEST(SignalCollectorTests, NetlistWithHdlnameParsing) {
    const auto& json = json_with_hdlname;
    const auto& signals = SignalCollector(
                              json_with_hdlname_top_module,
                              json_with_hdlname_top_instance,
                              json_with_hdlname_sig_path_prefix,
                              json_with_hdlname_liberty,
                              {}
    )
                              .collectFromJSON(json);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 3);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].signal_name, "dff_worker0.counter[0]");
    EXPECT_EQ(signals[0].cell_type, "$_DFFE_PP_");
    EXPECT_EQ(signals[0].width, 1);
    EXPECT_EQ(signals[0].hdlname, "dff_worker0 counter$dff");
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);

    EXPECT_EQ(signals[1].path_prefix, "top.worker");
    EXPECT_EQ(signals[1].signal_name, "dff_worker0.resp[0]");
    EXPECT_EQ(signals[1].cell_type, "$_DFFE_PP_");
    EXPECT_EQ(signals[1].width, 1);
    EXPECT_EQ(signals[1].hdlname, "dff_worker0 resp$dff");
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);

    EXPECT_EQ(signals[2].path_prefix, "top.worker");
    EXPECT_EQ(signals[2].signal_name, "dff_worker0.resp[32:13]");
    EXPECT_EQ(signals[2].cell_type, "$_DFFE_PP_");
    EXPECT_EQ(signals[2].width, 1);
    EXPECT_EQ(signals[2].hdlname, "dff_worker0 resp[32:13]$dff");
    EXPECT_EQ(signals[2].type, SignalType::REGISTER);
}
