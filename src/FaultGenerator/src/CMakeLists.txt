# SPDX-License-Identifier: Apache-2.0

add_library(FaultGenerationToolLib
  FaultStrategyFactory.cpp
  FaultStrategyRandom.cpp
  FaultStrategyWeibull.cpp
  GlobalOpts.cpp
  SignalCollector.cpp
)

target_include_directories(FaultGenerationToolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(FaultGenerationToolLib
  PRIVATE
    nlohmann_json::nlohmann_json
    absl::flags
    absl::flags_parse
    absl::strings)

