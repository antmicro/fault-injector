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

#include "FaultEvent.h"
#include "FaultStrategy.h"
#include "GlobalOpts.h"
#include "IsFlipFlopPredicate.h"
#include "LogUtils.h"
#include "Signal.h"
#include "SignalCollector.h"

#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <random>
#include <thread>
#include <vector>

struct TaskInput {
    std::shared_ptr<FaultStrategy> strategy;
    std::span<const Signal> signals;
    std::string output_file;
};

std::vector<TaskInput> generate_tasks(
    const std::shared_ptr<FaultStrategy> strategy,
    const std::vector<Signal>& signals,
    const std::string& root_path,
    std::uint64_t seed,
    std::uint64_t count
) {
    std::mt19937_64 seed_generator{seed};
    std::uniform_int_distribution<std::uint32_t> dist;
    std::vector<TaskInput> result;
    result.reserve(count);

    for (int i = 0; i < count; i++) {
        FaultStrategy::Config new_config = {
            .num_of_events = strategy->config.num_of_events,
            .seed = dist(seed_generator),
            .simulation_time = strategy->config.simulation_time
        };
        std::stringstream ss;
        ss << root_path << "/fault_campaign_" << new_config.seed << ".csv";
        result.emplace_back(strategy->copy_with(new_config), signals, ss.str());
    }
    return result;
}

void generate_single_campaign(const TaskInput& input) {
    LOG(INFO) << "call generate_single_campaign" << std::endl;
    const std::vector<FaultEvent> fault_events = input.strategy->generate(input.signals);
    std::ofstream of(input.output_file);
    if (!of) {
        std::cerr << "cannot open '" << input.output_file << "'\n"
                  << "Skipping campaign\n";
        return;
    }
    for (const FaultEvent& fault_event : fault_events) {
        of << fault_event.time << ',' << fault_event.signal_path << ',' << fault_event.bit_index
           << ',' << fault_event.type << '\n';
    }
}

void generate_many_campaigns(const GlobalOpts& opts, std::vector<Signal>&& signals) {
    LOG(INFO) << "call generate_many_campaigns" << std::endl;
    std::vector<TaskInput> tasks = generate_tasks(
        opts.strategy,
        signals,
        opts.fault_campaign_out,
        opts.strategy->config.seed,
        opts.campaign_number
    );
    auto num_workers =
        std::max(1u, std::min(opts.thread_number, std::thread::hardware_concurrency()));

    std::vector<std::future<void>> workers;

    auto start = tasks.cbegin();
    double distributed = 0.0;
    for (unsigned int i = 0; i < num_workers; ++i) {
        double should_be = ((double)opts.campaign_number * (double)(i + 1)) / num_workers;
        int dist = should_be - distributed;
        auto end = (i == num_workers - 1) ? tasks.cend() : std::next(start, dist);

        workers.push_back(std::async(std::launch::async, [start, end]() {
            std::for_each(start, end, generate_single_campaign);
        }));
        start = end;
        distributed += dist;
    }

    for (auto& fut : workers) {
        fut.wait();
    }
}

void create_directory(std::string_view path) {
    using namespace std::filesystem;
    if (exists(path) && is_directory(path)) {
        return;
    }
    std::stringstream ss;
    try {
        if (create_directories(path)) {
            return;
        }
        SEE_CHECK(false) << "Cannot create directory '" << path << "'";
    } catch (std::exception& e) {
        LOG(INFO) << e.what();
        SEE_CHECK(false) << "Cannot create directory '" << path << "'";
    }
}

int main(int argc, char* argv[]) {
    const GlobalOpts opts = GlobalOpts::parseCmdArgs(argc, argv);
    IsFlipFlop::ctor(opts);

    SEE_CHECK(opts.campaign_number >= 1) << "Cannot run less than one campaign";

    std::vector<Signal> signals =
        SignalCollector(opts.top_module, opts.top_instance, opts.sig_path_prefix)
            .collectFromFile(opts.netlist_path);

    if (opts.campaign_number == 1) {
        generate_single_campaign(
            {.strategy = opts.strategy, .signals = signals, .output_file = opts.fault_campaign_out}
        );

    } else {
        create_directory(opts.fault_campaign_out);
        generate_many_campaigns(opts, std::move(signals));
    }
}
