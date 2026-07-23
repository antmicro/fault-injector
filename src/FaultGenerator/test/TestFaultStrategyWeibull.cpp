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
#include "FaultStrategyWeibull.h"
#include "Signal.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

static std::vector<Signal> createSignals(size_t count) {
    std::vector<Signal> signals;
    signals.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        signals.push_back(
            {"signal_" + std::to_string(i),
             std::to_string(i),
             1024,
             std::nullopt,
             std::nullopt,
             SignalType::REGISTER}
        );
    }
    return signals;
}

TEST(WeibullGenerationTest, CountsWithinTolerance) {
    // From seu2.py
    constexpr std::uint64_t expected_counts[] = {2431964, 245205, 103683, 101457, 20256,
                                                 99086,   102352, 104023, 97811,  2741,
                                                 91813,   98614,  52001,  90941,  9825,
                                                 9659,    25,     26,     160635, 10396};
    constexpr size_t num_streams = sizeof(expected_counts) / sizeof(expected_counts[0]);
    constexpr std::uint64_t expected_total = 3832513;

    std::vector<Signal> signals = createSignals(100);

    FaultStrategy::Config config{0, 42, 0};

    WeibullConfig weibullConfig{
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

    WeibullStrategy strategy{config, weibullConfig};

    std::vector<FaultEvent> all_events;

    for (size_t i = 0; i < num_streams; ++i) {
        std::vector<FaultEvent> stream_events =
            strategy.generate(weibullConfig.streams[i], signals);
        std::uint64_t count = stream_events.size();
        std::uint64_t expected = expected_counts[i];
        all_events.insert(all_events.end(), stream_events.begin(), stream_events.end());

        double tolerance;
        if (expected > 100000) {
            tolerance = 0.05;
        } else if (expected > 1000) {
            tolerance = 0.10;
        } else if (expected > 100) {
            tolerance = 0.20;
        } else {
            tolerance = 5.0;
        }

        double diff =
            std::abs(static_cast<double>(count) - static_cast<double>(expected)) / expected;

        ASSERT_LE(diff, tolerance) << "stream #" << i << ": got " << count << ", expected "
                                   << expected << " (" << diff * 100.0 << "% diff)";
    }

    double total_diff =
        std::abs(static_cast<double>(all_events.size()) - static_cast<double>(expected_total)) /
        expected_total;
    EXPECT_LE(total_diff, 0.05) << "total events " << all_events.size() << ", expected "
                                << expected_total << " (" << total_diff * 100.0 << "% diff)";
}
