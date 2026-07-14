# SPDX-License-Identifier: Apache-2.0

add_library(FaultGenerationToolLib
  FaultStrategyBendel.cpp
  FaultStrategyFactory.cpp
  FaultStrategyRandom.cpp
  FaultStrategyWeibull.cpp
  GlobalOpts.cpp
  IsFlipFlopPredicate.cpp
  LibertyParser.cpp
  SignalCollector.cpp
)

target_include_directories(FaultGenerationToolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(FaultGenerationToolLib
  PRIVATE
    nlohmann_json::nlohmann_json
    absl::flags
    absl::flags_parse
    absl::strings
    absl::log
    absl::log_initialize
    absl::check
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,absl::log_flags>"
)

