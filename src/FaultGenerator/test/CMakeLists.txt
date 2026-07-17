# SPDX-License-Identifier: Apache-2.0

add_executable(TestFaultStrategyWeibull TestFaultStrategyWeibull.cpp)
target_link_libraries(TestFaultStrategyWeibull PRIVATE FaultGenerationToolLib)
add_test(NAME unit.generation.faultStrategy.weibull COMMAND TestFaultStrategyWeibull)

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
add_test(NAME unit.generation.gtest.configReading  COMMAND TestConfigReading)

add_executable(TestFaultStrategyBendel TestFaultStrategyBendel.cpp)
target_link_libraries(TestFaultStrategyBendel PRIVATE FaultGenerationToolLib GTest::gtest_main)
add_test(NAME unit.generation.faultStrategy.bendel COMMAND TestFaultStrategyBendel)


add_executable(TestLibertyParsing TestLibertyParsing.cpp)
target_link_libraries(TestLibertyParsing PRIVATE FaultGenerationToolLib GTest::gtest_main)
target_compile_definitions(TestLibertyParsing PRIVATE
  TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
add_test(NAME unit.generation.gtest.libertParsing COMMAND TestLibertyParsing)

add_executable(TestSignalCollector TestSignalCollector.cpp)
target_link_libraries(TestSignalCollector
  PRIVATE
    FaultGenerationToolLib
    nlohmann_json::nlohmann_json
    absl::log
    absl::log_initialize
    absl::check
    GTest::gtest_main
)
add_test(NAME TestSignalCollector COMMAND TestSignalCollector)


# Binary for testing LibertyParser
add_executable(LibertyParserBin LibertyParser.cpp)
target_link_libraries(LibertyParserBin
  PRIVATE
    absl::flags
    absl::flags_parse
    absl::strings
    FaultGenerationToolLib
)
add_test(NAME unit.generation.libertyParserOnExample
  COMMAND
    LibertyParserBin --input_path "${CMAKE_CURRENT_SOURCE_DIR}/example.lib"
)

