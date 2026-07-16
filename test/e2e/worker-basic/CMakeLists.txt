# SPDX-License-Identifier: Apache-2.0

set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(NETLIST_PATH "${WORK_DIR}/worker.json")
set(TEST_DIR "${FI_E2E_WORKER_DIR}")

# First produce the Yosys JSON netlist consumed by FaultGenerationTool.
fi_add_yosys_json(fi_e2e_worker_basic_netlist
  SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/worker_json.tcl"
  SOURCES "${FI_E2E_WORKER_DIR}/dff_worker.v"
)

add_subdirectory(baseline)
add_subdirectory(bendel)
add_subdirectory(weibull)
add_subdirectory(random)
