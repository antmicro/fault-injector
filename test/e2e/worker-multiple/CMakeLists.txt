# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_worker_multiple)
set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(NETLIST_PATH "${WORK_DIR}/worker.json")
set(CAMPAIGN_DIR "${WORK_DIR}/fault_campaign_out")
set(CAMPAIGN_STAMP "${WORK_DIR}/fault_campaign_out.stamp")
set(FAULT_CAMPAIGN_OUT "${CAMPAIGN_DIR}")
set(TEST_DIR "${FI_E2E_WORKER_DIR}")

fi_add_yosys_json(fi_e2e_worker_multiple_netlist
  SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/worker_json.tcl"
  SOURCES "${FI_E2E_WORKER_DIR}/dff_worker.v"
)

set(SIMULATION_DIR "${WORK_DIR}/campaign")
fi_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.json.in")
fi_add_fault_campaign(fi_e2e_worker_multiple_campaigns
  OUTPUT "${CAMPAIGN_STAMP}"
  DEPENDS "${NETLIST_PATH}"
)

fi_add_multi_campaign_runs(fi_e2e_worker_multiple_campaign_runs
  DEPENDS "${CAMPAIGN_STAMP}"
)

fi_add_ctest_scenario(e2e.worker.multiple ${SCENARIO_TARGET}
  DEPENDS
    fi_e2e_worker_multiple_campaign_runs
)

