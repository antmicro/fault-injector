# SPDX-License-Identifier: Apache-2.0

add_library(FaultInjector STATIC FaultInjector.cpp)

if(NOT VERILATOR_ROOT)
	message(FATAL_ERROR "VERILATOR_ROOT is not set")
endif()

target_include_directories(FaultInjector PRIVATE
	${VERILATOR_ROOT}/include
	${VERILATOR_ROOT}/include/vltstd
	../../third_party/csv
	src/
)
