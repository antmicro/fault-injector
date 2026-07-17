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

#include "Event.h"

#include <gtest/gtest.h>

namespace {

TEST(EventParsing, ParsesValidSetEvent) {
    auto result = fin::parseEvent("100,TOP.test_signal,3,set");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->time, 100);
    EXPECT_EQ(result->sig_path, "TOP.test_signal");
    EXPECT_EQ(result->bit_idx, 3);
    EXPECT_EQ(result->type, fin::Event::Type::SingleEventTransientUpset);
    EXPECT_EQ(result->vpi_handle, nullptr);
    EXPECT_FALSE(result->vpi_value.has_value());
}

TEST(EventParsing, ParsesValidSeuEvent) {
    auto result = fin::parseEvent("50,TOP.another_sig,7,seu");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->time, 50);
    EXPECT_EQ(result->sig_path, "TOP.another_sig");
    EXPECT_EQ(result->bit_idx, 7);
    EXPECT_EQ(result->type, fin::Event::Type::SingleEventUpset);
}

TEST(EventParsing, ReturnsNulloptForUnknownType) {
    auto result = fin::parseEvent("100,TOP.sig,0,unknown");
    EXPECT_FALSE(result.has_value());
}

}  // namespace
