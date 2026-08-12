# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_veer_basic_baseline)
set(SIMULATION_DIR "${WORK_DIR}/baseline")

fi_add_verilated_sim(fi_e2e_veer_basic_baseline_sim
  VEER_MODE
  NO_MAIN NO_BUILD
  VERILATOR_EXTRA_ARGS "${VERILATOR_EXTRA_ARGS}"
)
fi_add_sim_run(fi_e2e_veer_basic_baseline_run
  VEER_MODE
  DEPENDS fi_e2e_veer_basic_baseline_sim
)

fi_add_ctest_scenario(e2e.veer.basic.baseline ${SCENARIO_TARGET}
  DEPENDS fi_e2e_veer_basic_baseline_run
)

