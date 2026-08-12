# SPDX-License-Identifier: Apache-2.0

set(E2E_VEER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(SYNTH_HDL_FRONTEND "slang")
define_veer_testbench_vars()

set(JSON_NETLIST "${WORK_DIR}/${DESIGN_TOP}_post_synth.json")
set(VERILOG_NETLIST "${WORK_DIR}/${DESIGN_TOP}_post_synth.v")
set(TB_TOP "Vtb_top")
set(VEER_NETLIST_CACHE_PATH "" CACHE FILEPATH "Existing VeeR JSON netlist to reuse for e2e tests")

if(VEER_NETLIST_CACHE_PATH)
  if(NOT EXISTS "${VEER_NETLIST_CACHE_PATH}")
    message(FATAL_ERROR "VEER_NETLIST_CACHE_PATH does not exist: ${VEER_NETLIST_CACHE_PATH}")
  endif()
  message(STATUS "Using cached VeeR netlist: ${VEER_NETLIST_CACHE_PATH}")
  add_custom_command(
    OUTPUT "${JSON_NETLIST}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${WORK_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${VEER_NETLIST_CACHE_PATH}" "${JSON_NETLIST}"
    DEPENDS "${VEER_NETLIST_CACHE_PATH}"
    VERBATIM
  )
  add_custom_target(fi_e2e_veer_basic_netlist DEPENDS "${JSON_NETLIST}")
else()
  message(STATUS "VEER_NETLIST_CACHE_PATH is not set; VeeR netlist will be generated with Yosys")
  fi_add_yosys_json(fi_e2e_veer_basic_netlist
    ENV "RV_ROOT=${VEER_ROOT}"
  )
endif()

add_subdirectory(baseline)
add_subdirectory(random)
