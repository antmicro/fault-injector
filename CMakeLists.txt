# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(FaultInjectionSuite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include(CTest)

if(BUILD_TESTING)
  include(cmake/common.cmake)
  fi_require_e2e_tools()
endif()

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(json)

FetchContent_Declare(
    absl
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20250512.0
)

FetchContent_MakeAvailable(absl)

if(BUILD_TESTING)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(src/FaultGenerator)
add_subdirectory(src/FaultInjector)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
