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

#include "SignalCollector.h"

#include "Cell.h"
#include "IsFlipFlopPredicate.h"
#include "LogUtils.h"
#include "Signal.h"

#include <bitset>
#include <fstream>
#include <sstream>
#include <string_view>

#include <absl/strings/str_cat.h>

namespace {

/** How many bits parameter.WIDTH takes as a string */
constexpr unsigned int SIGNAL_WIDTH_PARAMETER_LENGTH = 32;
unsigned int getSignalWidth(std::string_view width_bits) {
    return std::bitset<SIGNAL_WIDTH_PARAMETER_LENGTH>(std::string(width_bits)).to_ulong();
}

std::string combineSignalPath(std::string_view current, std::string_view next) {
    if (current.empty()) {
        return std::string{next};
    }
    return absl::StrCat(current, ".", next);
}

}  // namespace

std::vector<Signal> SignalCollector::collectFromFile(const std::string& netlist_filepath) const {
    std::ifstream netlist_file(netlist_filepath);
    SEE_PCHECK(netlist_file) << "Cannot access netlist file '" << netlist_filepath << "'";

    try {
        nlohmann::json netlist_json = nlohmann::json::parse(netlist_file);
        return collectFromJSON(netlist_json);
    } catch (...) {
        SEE_CHECK(false) << "Malformed netlist file '" << netlist_filepath << "'";
    }
}

std::vector<Signal> SignalCollector::collectFromJSON(const nlohmann::json& json) const {
    std::vector<Module> collected_modules = collectModules(json);

    int top_module_index = findTopModule(collected_modules);

    std::string starting_path = combineSignalPath(prefix_path, top_instance);

    std::vector<Signal> collected_signals;
    recursivelyCollectSignals(
        collected_signals, starting_path, collected_modules, collected_modules[top_module_index]
    );

    SEE_CHECK(!collected_signals.empty()) << "No signals found, cannot generate faults";
    return collected_signals;
}

int SignalCollector::findTopModule(std::vector<SignalCollector::Module>& modules) const {
    for (unsigned int index = 0; index < modules.size(); ++index) {
        if (modules[index].name == top_module) {
            return index;
        }
    }
    VLOG(3) << dumpAllModules(modules);
    VLOG(3) << "Top module: " << top_module;
    SEE_CHECK(false) << "Top module not found. Cannot generate faults without signals.";
}

std::string SignalCollector::dumpAllModules(const std::vector<Module>& modules) {
    std::stringstream ss;
    for (const auto& mod : modules) {
        ss << "Module: " << mod.name << "\n";
        ss << "  Signals:\n";
        for (const auto& signal : mod.signals) {
            ss << "    " << signal << "\n";
        }
        for (const auto& [instance_name, module_index] : mod.child_modules) {
            ss << "  Child modules:\n";
            ss << "    { " << instance_name << ": " << modules[module_index].name << " }\n";
        }
        ss << "\n";
    }
    return ss.str();
}

void SignalCollector::collectCell(Module& mod, const Cell& cell, const nlohmann::json& json) {
    if (IsFlipFlop::check(cell)) {
        LOG(INFO) << "Cell '" << cell.name << "' is a flip-flop";
        if (!json.contains("parameters")) {
            LOG(WARNING) << "Cell '" << cell.name
                         << "' has no property 'parameters', Skipping cell.";
            return;
        }
        const auto& params = json["parameters"];
        unsigned int width;
        if (!params.contains("WIDTH")) {
            LOG(WARNING) << "Cell '" << cell.name
                         << "' has no property 'parameters.WIDTH', setting value to 1.";
            // FIXME: During synthesis, signal with WIDTH n is split into n signals
            // (presumably with WIDTH 1), The question is if we should treat them as
            // separate signals, or to somehow join them back together
            width = 1;
        } else {
            width = getSignalWidth(params["WIDTH"].get<std::string_view>());
        }
        mod.signals.emplace_back(cell.getPath(), width, SignalType::REGISTER);
        VLOG(3) << "Found signal " << mod.signals.back();
    } else {
        VLOG(1) << "Cell '" << cell.name << "' is not a flip-flop. Skipping";
    }
}

std::vector<SignalCollector::Module> SignalCollector::collectModules(const nlohmann::json& json) {
    std::unordered_map<std::string, unsigned int> existing_modules;
    std::vector<Module> modules;

    SEE_CHECK(json.contains("modules") && json["modules"].is_object()) << "Malformed netlist json";

    const auto& modules_json = json["modules"];
    // Fill existing_modules
    for (const auto& [name, value] : modules_json.items()) {
        if (value.contains("cells") && value["cells"].is_object()) {
            existing_modules[name] = modules.size();
            modules.push_back({name});
        }
    }

    for (const auto& [module_key, module_value] : modules_json.items()) {
        Module mod;
        mod.name = module_key;

        if (!module_value.contains("cells") || !module_value["cells"].is_object()) {
            LOG(WARNING) << "Module '" << module_key << "' contains no cells. Skipping module.";
            continue;
        }

        for (const auto& [cell_key, cell_value] : module_value["cells"].items()) {
            Cell cell(cell_key, cell_value);

            if (auto it = existing_modules.find(cell.type); it != existing_modules.end()) {
                LOG(INFO) << "Cell's '" << cell.name << "' is child of module '" << it->first
                          << "'";
                mod.child_modules.emplace_back(cell_key, it->second);
            } else {
                collectCell(mod, cell, cell_value);
            }
        }
        modules[existing_modules[mod.name]] = std::move(mod);
    }
    SEE_CHECK(!modules.empty()) << "No modules found";
    return modules;
}

void SignalCollector::recursivelyCollectSignals(
    std::vector<Signal>& collected_signals,
    std::string_view current_path,
    const std::vector<Module>& modules,
    const Module& mod
) const {
    VLOG(2) << "Collecting signals for '" << mod.name << "' under prefix: " << current_path;
    for (const Signal& signal : mod.signals) {
        collected_signals.emplace_back(
            combineSignalPath(current_path, signal.signal_path), signal.num_of_bits, signal.type
        );
    }

    for (const auto& [instance_name, module_index] : mod.child_modules) {
        std::string next_path = combineSignalPath(current_path, instance_name);
        recursivelyCollectSignals(collected_signals, next_path, modules, modules[module_index]);
    }
}
