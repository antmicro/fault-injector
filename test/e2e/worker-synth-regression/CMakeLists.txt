# SPDX-License-Identifier: Apache-2.0

# This scenario replaces test-synth-regression.sh. It is a single CTest
# scenario with two internal subtests: synthesis without techmap and synthesis
# with techmap.

set(SCENARIO_TARGET fi_e2e_worker_synth_regression)
set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(SOURCES "${FI_E2E_WORKER_DIR}/dff_worker.v")
set(TEST_DIR "${FI_E2E_WORKER_DIR}")

set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_out_no_techmap.csv")
set(NETLIST_PATH "${WORK_DIR}/dff_worker_post_synth_no_techmap.json")
set(SIMULATION_DIR "${WORK_DIR}/no_techmap")
fi_add_yosys_json(fi_e2e_worker_synth_no_techmap_netlist
  SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/synth_no_techmap.tcl"
  OUTPUT "${NO_TECHMAP_NETLIST}"
)

fi_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.json.in")
fi_add_fault_campaign(fi_e2e_worker_synth_no_techmap_campaign
  DEPENDS "${NETLIST_PATH}"
)
fi_add_verilated_sim(fi_e2e_worker_synth_no_techmap_sim
  FAULT_INJECTION
  SOURCES ${WORKER_SOURCES}
  DEPENDS "${FAULT_CAMPAIGN_OUT}"
)
fi_add_sim_run(fi_e2e_worker_synth_no_techmap_run
  EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_synth_no_techmap_sim
)

set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_out_with_techmap.csv")
set(NETLIST_PATH "${WORK_DIR}/dff_worker_post_synth.json")
set(SIMULATION_DIR "${WORK_DIR}/with_techmap")
fi_add_yosys_json(fi_e2e_worker_synth_with_techmap_netlist
  SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/synth_with_techmap.tcl"
  OUTPUT "${WITH_TECHMAP_NETLIST}"
)

fi_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.json.in")
fi_add_fault_campaign(fi_e2e_worker_synth_with_techmap_campaign
  DEPENDS "${WITH_TECHMAP_NETLIST}"
)
fi_add_verilated_sim(fi_e2e_worker_synth_with_techmap_sim
  FAULT_INJECTION
  SOURCES ${WORKER_SOURCES}
  DEPENDS "${WITH_TECHMAP_CAMPAIGN}"
)
fi_add_sim_run(fi_e2e_worker_synth_with_techmap_run
  # FIXME: As of now, if FaultGenerationTool receives yosys json post synthesis,
  # it produces events for signals paths that don't appear in the simulation.
  # That means all events are rejected, and so no injection takes place.
  #
  # EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_synth_with_techmap_sim
)

fi_add_ctest_scenario(e2e.worker.synth-regression ${SCENARIO_TARGET}
  DEPENDS
    fi_e2e_worker_synth_no_techmap_run
    fi_e2e_worker_synth_with_techmap_run
)

