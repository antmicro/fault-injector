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

#include "FaultCampaignWriter.h"
#include "FaultEvent.h"
#include "FaultEventsSignalFormatter.h"
#include "FaultStrategy.h"
#include "FaultStrategyBendel.h"
#include "FaultStrategyRandom.h"
#include "FaultStrategyWeibull.h"

#include "TestUtils.h"

#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <string>
#include <vector>

const FaultStrategy::Config config{
    .num_of_events = 10,
    .seed = 2137,
    .simulation_time = 1000,
    .thread_number = 4
};

TEST(FaultGenerationShouldBeDeterministic, WeibullStrategy) {
    std::vector<Signal> signals = createSignals(10);

    WeibullConfig weibull_config = {
        .streams =
            {WeibullConfig::Stream{.let = 67.7 * 1e5, .flux_phi = 9.15e3 * 1e4, .max_time = 1094},
             WeibullConfig::Stream{.let = 67.7 * 1e5, .flux_phi = 1.01e3 * 1e4, .max_time = 996},
             WeibullConfig::Stream{.let = 67.7 * 1e5, .flux_phi = 1.04e3 * 1e4, .max_time = 409},
             WeibullConfig::Stream{.let = 67.7 * 1e5, .flux_phi = 1.05e3 * 1e4, .max_time = 399},
             WeibullConfig::Stream{.let = 67.7 * 1e5, .flux_phi = 5.04e2 * 1e4, .max_time = 166},
             WeibullConfig::Stream{.let = 40.4 * 1e5, .flux_phi = 1.01e3 * 1e4, .max_time = 536},
             WeibullConfig::Stream{.let = 40.4 * 1e5, .flux_phi = 1.01e3 * 1e4, .max_time = 551},
             WeibullConfig::Stream{.let = 32.6 * 1e5, .flux_phi = 1.58e3 * 1e4, .max_time = 417},
             WeibullConfig::Stream{.let = 32.6 * 1e5, .flux_phi = 1.51e3 * 1e4, .max_time = 411},
             WeibullConfig::Stream{.let = 32.6 * 1e5, .flux_phi = 1.45e3 * 1e4, .max_time = 12},
             WeibullConfig::Stream{.let = 20.4 * 1e5, .flux_phi = 2.00e3 * 1e4, .max_time = 433},
             WeibullConfig::Stream{.let = 20.4 * 1e5, .flux_phi = 2.05e3 * 1e4, .max_time = 452},
             WeibullConfig::Stream{.let = 10.2 * 1e5, .flux_phi = 2.32e3 * 1e4, .max_time = 433},
             WeibullConfig::Stream{.let = 10.2 * 1e5, .flux_phi = 2.77e3 * 1e4, .max_time = 636},
             WeibullConfig::Stream{.let = 3.0 * 1e5, .flux_phi = 5.03e3 * 1e4, .max_time = 201},
             WeibullConfig::Stream{.let = 3.0 * 1e5, .flux_phi = 5.11e3 * 1e4, .max_time = 197},
             WeibullConfig::Stream{.let = 1.1 * 1e5, .flux_phi = 7.60e3 * 1e4, .max_time = 133},
             WeibullConfig::Stream{.let = 1.1 * 1e5, .flux_phi = 8.17e3 * 1e4, .max_time = 124},
             WeibullConfig::Stream{.let = 32.6 * 1e5, .flux_phi = 9.99e3 * 1e4, .max_time = 102},
             WeibullConfig::Stream{.let = 32.6 * 1e5, .flux_phi = 5.17e1 * 1e4, .max_time = 1275}}
    };

    WeibullStrategy strategy{config, weibull_config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    FaultCampaignWriter::FaultFormatter formatter(FaultEventsSignalFormatter("", signals));
    std::stringstream actual;
    FaultCampaignWriter(formatter).write(actual, stream_events);
    std::ifstream expected(std::string(TEST_DATA_DIR) + "/weibull_campaign.csv.out");

    testStreams(expected, actual);
}

TEST(FaultGenerationShouldBeDeterministic, BendelStrategy) {
    std::vector<Signal> signals = createSignals(10);

    BendelConfig bendel_config =
        {.streams = {
             BendelConfig::Stream{
                 .name = "run55",
                 .energy = 20.0 * 1e6,
                 .flux_phi = 1.12e8 * 1e4,
                 .fluence = 1e10 * 1e4
             },
             BendelConfig::Stream{
                 .name = "run52",
                 .energy = 40.0 * 1e6,
                 .flux_phi = 1.19e8 * 1e4,
                 .fluence = 1e10 * 1e4
             },
             BendelConfig::Stream{
                 .name = "run47",
                 .energy = 60.0 * 1e6,
                 .flux_phi = 9.17e7 * 1e4,
                 .fluence = 1e10 * 1e4
             },
         }};

    BendelStrategy strategy{config, bendel_config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    FaultCampaignWriter::FaultFormatter formatter(FaultEventsSignalFormatter("", signals));
    std::stringstream actual;
    FaultCampaignWriter(formatter).write(actual, stream_events);
    std::ifstream expected(std::string(TEST_DATA_DIR) + "/bendel_campaign.csv.out");

    testStreams(expected, actual);
}

TEST(FaultGenerationShouldBeDeterministic, RandomStrategy) {
    std::vector<Signal> signals = createSignals(10);

    RandomStrategy strategy{config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    FaultCampaignWriter::FaultFormatter formatter(FaultEventsSignalFormatter("", signals));
    std::stringstream actual;
    FaultCampaignWriter(formatter).write(actual, stream_events);
    std::ifstream expected(std::string(TEST_DATA_DIR) + "/random_campaign.csv.out");

    testStreams(expected, actual);
}
