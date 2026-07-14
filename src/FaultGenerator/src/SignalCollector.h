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

#pragma once

#include <string_view>

#include <nlohmann/json.hpp>

#include "Signal.h"

class SignalCollector {
    std::string_view top_module;
    std::string_view top_instance;
    std::string_view prefix_path;

   public:
    SignalCollector(std::string_view top_module,
                    std::string_view top_instance,
                    std::string_view prefix_path)
        : top_module(top_module), top_instance(top_instance), prefix_path(prefix_path) {}

    std::vector<Signal> collectFromFile(const std::string& netlist_filepath) const;

   private:
    struct Module {
        std::string name;

        // Name of the module instance and index to the module inside modules array.
        std::vector<std::pair<std::string, unsigned int>> child_modules;

        std::vector<Signal> signals;
    };

    static std::vector<Module> collectModules(const nlohmann::json& json);
    static std::string dumpAllModules(const std::vector<Module>& modules);
    int findTopModule(std::vector<Module>& modules) const;
    void recursivelyCollectSignals(std::vector<Signal>& collected_signals,
                                   std::string_view current_path,
                                   const std::vector<Module>& modules,
                                   const Module& module) const;
    std::vector<Signal> collectFromJSON(const nlohmann::json& json) const;
};
