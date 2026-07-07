# SPDX-License-Identifier: Apache-2.0

# Test 1
add_executable(TestFaultStrategyWeibull TestFaultStrategyWeibull.cpp)
target_link_libraries(TestFaultStrategyWeibull PRIVATE FaultGenerationToolLib)
add_test(NAME TestFaultStrategyWeibull COMMAND TestFaultStrategyWeibull)

# Test 2
add_executable(TestConfigReading TestConfigReading.cpp)
target_link_libraries(TestConfigReading
  PRIVATE
    FaultGenerationToolLib
    nlohmann_json::nlohmann_json
    absl::flags
    absl::flags_parse
    absl::strings
    GTest::gtest_main
)
add_test(NAME TestConfigReading  COMMAND TestConfigReading)

# Test 3
add_executable(TestLibertyParsing TestLibertyParsing.cpp)
target_link_libraries(TestLibertyParsing PRIVATE FaultGenerationToolLib GTest::gtest_main)
target_compile_definitions(TestLibertyParsing PRIVATE
  TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
add_test(NAME TestLibertyParsing COMMAND TestLibertyParsing)

# Binary for testing LibertyParser
add_executable(LibertyParserBin LibertyParser.cpp)
target_link_libraries(LibertyParserBin
  PRIVATE
    absl::flags
    absl::flags_parse
    absl::strings
    FaultGenerationToolLib
)
add_test(NAME TestLibertyParserOnExample
  COMMAND
    LibertyParserBin --input_path "${CMAKE_CURRENT_SOURCE_DIR}/example.lib"
)

