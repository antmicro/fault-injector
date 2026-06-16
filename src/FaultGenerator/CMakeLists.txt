# SPDX-License-Identifier: Apache-2.0

project(FaultGenerationTool)

add_subdirectory(src)

# Build exec
add_executable(FaultGenerationTool main.cpp)
target_link_libraries(FaultGenerationTool
  PRIVATE
    FaultGenerationToolLib
    nlohmann_json::nlohmann_json)

# Include unit tests
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

