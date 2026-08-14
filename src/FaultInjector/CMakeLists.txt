# SPDX-License-Identifier: Apache-2.0

add_library(faultergeist-inject STATIC FaultInjector.cpp src/EventParser.cpp)

target_include_directories(faultergeist-inject PRIVATE
  src/
)

# Include unit tests
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
