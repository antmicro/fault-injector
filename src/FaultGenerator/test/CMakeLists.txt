# SPDX-License-Identifier: Apache-2.0

add_library(FaultGeneratorTestMain STATIC FaultGeneratorTestMain.cpp)
target_link_libraries(FaultGeneratorTestMain
  PUBLIC
    GTest::gtest
    FaultGenerationToolLib
  PRIVATE
    absl::flags_parse
)

add_executable(TestFaultStrategyWeibull TestFaultStrategyWeibull.cpp)
target_link_libraries(TestFaultStrategyWeibull
  PRIVATE
    FaultGeneratorTestMain
)
add_test(NAME unit.generation.faultStrategy.weibull COMMAND TestFaultStrategyWeibull)

add_executable(TestConfigReading TestConfigReading.cpp)
target_link_libraries(TestConfigReading
  PRIVATE
    FaultGeneratorTestMain
    absl::flags
    absl::flags_parse
    absl::strings
)
add_test(NAME unit.generation.gtest.configReading  COMMAND TestConfigReading)

add_executable(TestFaultStrategyBendel TestFaultStrategyBendel.cpp)
target_link_libraries(TestFaultStrategyBendel PRIVATE FaultGeneratorTestMain)
add_test(NAME unit.generation.faultStrategy.bendel COMMAND TestFaultStrategyBendel)

add_executable(TestLibertyParsing TestLibertyParsing.cpp)
target_link_libraries(TestLibertyParsing PRIVATE FaultGeneratorTestMain)
target_compile_definitions(TestLibertyParsing PRIVATE
  TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
add_test(NAME unit.generation.gtest.libertParsing COMMAND TestLibertyParsing)

add_executable(TestSignalCollector TestSignalCollector.cpp)
target_link_libraries(TestSignalCollector
  PRIVATE
    FaultGeneratorTestMain
)
add_test(NAME unit.generation.gtest.signalCollector COMMAND TestSignalCollector)

# Binary for testing LibertyParser
add_executable(LibertyParserBin LibertyParser.cpp)
target_link_libraries(LibertyParserBin
  PRIVATE
    FaultGenerationToolLib
    absl::flags
    absl::flags_parse
    absl::strings
)
add_test(NAME unit.generation.libertyParserOnExample
  COMMAND
    LibertyParserBin --input_path "${CMAKE_CURRENT_SOURCE_DIR}/example.lib"
)
