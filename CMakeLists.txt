# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(FaultInjectionSuite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

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

include(CTest)

option(VERILATOR_ROOT "VERILATOR_ROOT [required]" NO)

add_subdirectory(src/FaultGenerator)
add_subdirectory(src/FaultInjector)
