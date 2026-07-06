# SPDX-License-Identifier: Apache-2.0

add_library(FaultInjector STATIC FaultInjector.cpp src/Event.cpp)

target_include_directories(FaultInjector PRIVATE
	src/
)

# Include unit tests
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
