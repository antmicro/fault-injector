# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_worker_synth_hierarchical)
set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/config.vlt"
  "${FI_E2E_WORKER_DIR}/top.v"
  "${FI_E2E_WORKER_DIR}/worker.v"
  "${FI_E2E_WORKER_DIR}/comb_worker.v"
  "${FI_E2E_WORKER_DIR}/dff_worker.v"
)
set(TEST_DIR "${FI_E2E_WORKER_DIR}")
set(PDK_PATH "${WORKER_PDK_PATH}")

set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_hierarchical.csv")
set(NETLIST_PATH "${WORK_DIR}/worker_post_synth.json")
set(SIMULATION_DIR "${WORK_DIR}/synth_hierarchical")
fi_add_yosys_json(fi_e2e_worker_synth_hierarchical_netlist
  SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/synth_hierarchical.tcl"
  OUTPUT "${NETLIST_PATH}"
)

fi_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.json.in")
fi_add_fault_campaign(fi_e2e_worker_synth_hierarchical_campaign
  DEPENDS "${NETLIST_PATH}"
)
fi_add_verilated_sim(fi_e2e_worker_synth_hierarchical_sim
  FAULT_INJECTION
  SOURCES ${SOURCES}
  DEPENDS "${FAULT_CAMPAIGN_OUT}"
)
fi_add_sim_run(fi_e2e_worker_synth_hierarchical_run
  EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_synth_hierarchical_sim
)

fi_add_ctest_scenario(e2e.worker.synth-hierarchical ${SCENARIO_TARGET}
  DEPENDS
    fi_e2e_worker_synth_hierarchical_run
)
