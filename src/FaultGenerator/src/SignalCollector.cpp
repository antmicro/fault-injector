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

#include <bitset>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include <absl/strings/str_cat.h>

#include "Signal.h"

namespace {

// FIXME use .lib for that
bool isFlipFlop(std::string_view cell_name) {
    static std::unordered_set<std::string_view> ff_types = {
        "$dff",   "$adff",   "$aldff",  "$dffsr", "$sdff",  "$dffe",
        "$adffe", "$aldffe", "$dffsre", "$sdffe", "$sdffce"};
    std::string_view cell_ff_type = cell_name.substr(cell_name.find_last_of('$'));
    return ff_types.contains(cell_ff_type);
}

unsigned int getSignalWidth(const std::string& width_bits) {
    return std::bitset<32>(width_bits).to_ulong();
}

std::string_view getFFCellName(std::string_view cell_name) {
    return cell_name.substr(0, cell_name.find('$'));
}

std::string combineSignalPath(std::string_view current, std::string_view next) {
    if (current.empty()) {
        return std::string{next};
    }
    return absl::StrCat(current, ".", next);
}

static constexpr int INVALID_TOP_MODULE_INDEX = -1;

}  // namespace

std::vector<Signal> SignalCollector::collectFromFile(const std::string& netlist_filepath) const {
    std::ifstream netlist_file(netlist_filepath);
    if (!netlist_file) {
        std::cerr << "cannot access '" << netlist_filepath << "': No such file or directory\n";
        std::exit(1);
    }

    nlohmann::json netlist_json = nlohmann::json::parse(netlist_file);
    return collectFromJSON(netlist_json);
}

std::vector<Signal> SignalCollector::collectFromJSON(const nlohmann::json& json) const {
    std::vector<Module> collected_modules = collectModules(json);
    int top_module_index = findTopModule(collected_modules);
    if (top_module_index == INVALID_TOP_MODULE_INDEX) {
        return {};
    }
    std::string starting_path = combineSignalPath(prefix_path, top_instance);
    std::vector<Signal> collected_signals;
    recursivelyCollectSignals(collected_signals, starting_path, collected_modules,
                              collected_modules[top_module_index]);
    return collected_signals;
}

std::vector<SignalCollector::Module> SignalCollector::collectModules(
    const nlohmann::json& json) const {
    std::unordered_map<std::string, unsigned int> existing_modules;
    std::vector<Module> modules;
    for (const auto& [module_key, module_value] : json["modules"].items()) {
        Module module;
        module.name = module_key;
        for (const auto& [cell_key, cell_value] : module_value["cells"].items()) {
            const std::string& type = cell_value["type"].get<std::string>();
            // TODO: it assumes child modules were declared earlier in the netlist
            // json. For more robustness, this should collect module names and then
            // resolve indices after all modules were gathered.
            if (auto it = existing_modules.find(type); it != existing_modules.end()) {
                module.child_modules.emplace_back(cell_key, it->second);
            } else if (isFlipFlop(cell_key)) {
                const auto& params = cell_value["parameters"];
                const auto& width = params["WIDTH"];
                module.signals.emplace_back(
                    std::string{getFFCellName(cell_key)}, getSignalWidth(width.get<std::string>()),
                    isFlipFlop(cell_key) ? SignalType::REGISTER : SignalType::WIRE);
            }
        }
        existing_modules[module.name] = modules.size();
        modules.emplace_back(std::move(module));
    }
    return modules;
}

int SignalCollector::findTopModule(std::vector<SignalCollector::Module>& modules) const {
    for (unsigned int index = 0; index < modules.size(); ++index) {
        if (modules[index].name == top_module) {
            return index;
        }
    }
    std::cerr << "Unable to find given top module!\n";
    return INVALID_TOP_MODULE_INDEX;
}

void SignalCollector::recursivelyCollectSignals(std::vector<Signal>& collected_signals,
                                                std::string_view current_path,
                                                const std::vector<Module>& modules,
                                                const Module& module) const {
    for (const Signal& signal : module.signals) {
        collected_signals.emplace_back(combineSignalPath(current_path, signal.signal_path),
                                       signal.num_of_bits, signal.type);
    }

    for (const auto& [instance_name, module_index] : module.child_modules) {
        std::string next_path = combineSignalPath(current_path, instance_name);
        recursivelyCollectSignals(collected_signals, next_path, modules, modules[module_index]);
    }
}
