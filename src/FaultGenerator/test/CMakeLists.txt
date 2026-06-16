# SPDX-License-Identifier: Apache-2.0

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)

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
