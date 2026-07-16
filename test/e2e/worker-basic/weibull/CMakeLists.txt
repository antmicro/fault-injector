# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_worker_basic_weibull)
set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/weibull_campaign.csv")
set(SIMULATION_DIR "${WORK_DIR}/weibull")

fi_configure_file("${CMAKE_CURRENT_LIST_DIR}/config.json.in")
fi_add_fault_campaign(fi_e2e_worker_basic_weibull_campaign
  DEPENDS fi_e2e_worker_basic_netlist
)
fi_add_verilated_sim(fi_e2e_worker_basic_weibull_sim
  FAULT_INJECTION
  SOURCES ${WORKER_SOURCES}
  DEPENDS "${FAULT_CAMPAIGN_OUT}"
)
fi_add_sim_run(fi_e2e_worker_basic_weibull_run
  EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_basic_weibull_sim
)

fi_add_ctest_scenario(e2e.worker.basic.weibull ${SCENARIO_TARGET}
  DEPENDS fi_e2e_worker_basic_weibull_run
)

