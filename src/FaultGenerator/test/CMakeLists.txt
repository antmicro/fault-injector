# SPDX-License-Identifier: Apache-2.0

# Test 1
add_executable(TestFaultStrategyWeibull TestFaultStrategyWeibull.cpp)
target_link_libraries(TestFaultStrategyWeibull PRIVATE FaultGenerationToolLib)
add_test(NAME TestFaultStrategyWeibull COMMAND TestFaultStrategyWeibull)

# Test 2
add_executable(TestFaultStrategyFactory TestFaultStrategyFactory.cpp)
target_link_libraries(TestFaultStrategyFactory
  PRIVATE
    FaultGenerationToolLib
    nlohmann_json::nlohmann_json
    absl::flags
    absl::flags_parse
    absl::strings
    GTest::gtest_main
)
add_test(NAME TestFaultStrategyFactory  COMMAND TestFaultStrategyFactory )
