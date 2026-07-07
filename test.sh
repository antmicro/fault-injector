#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

CURRENT_DIR="$(dirname $(readlink -f ${BASH_SOURCE[0]}))"
TEST_DIR="$CURRENT_DIR/test/worker"

if [[ -z "$VERILATOR_ROOT" ]]
then
  echo "This script depends on VERILATOR_ROOT variable being set."
  exit 1
fi

cmake -B build \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DCMAKE_BUILD_TYPE=Debug

cmake --build build -j

yosys <<EOF
	read_verilog $TEST_DIR/dff_worker.v
	proc
	rename -wire
	write_json worker.json
	exit
EOF

./build/src/FaultGenerator/FaultGenerationTool \
	--sig_path_prefix="top" \
	--top_module="dff_worker" \
	--top_instance="worker" \
	--netlist_path="worker.json" \
	--simulation_time=10 \
	--num_of_events=10

rm -rf ./obj_dir

"$VERILATOR_ROOT"/bin/verilator \
	--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v -j "$(nproc)" -CFLAGS '-g'

# TODO go back to normal printing when excessive logs (from unrecognized signal) are fixed
# Print logs, only when simulation failed
./obj_dir/Vtop | grep -v "Ignoring the event" -B 1 | tee sim_no_fault.log >/dev/null || cat sim_no_fault.log
grep -q "Mismatch" sim_no_fault.log && printf "==========\nFailure, simulation should pass\n==========\n" && exit 1

"$VERILATOR_ROOT"/bin/verilator \
	--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v ./src/FaultInjector/FaultInjector.sv -LDFLAGS "-L$(pwd)/build/src/FaultInjector -lFaultInjector" -j "$(nproc)" -CFLAGS '-g' -DFAULT_INJECTION_ENABLE -DFAULT_INJECTION_CAMPAIGN_FILE="\"fault_campaign_out.csv\""

# Print logs, only when simulation succeeded (when it should have failed)
./obj_dir/Vtop | grep -v "Ignoring the event" -B 1 | tee sim_fault.log  >/dev/null && cat sim_fault.log
grep -q "Mismatch" sim_fault.log && printf "==========\nSuccess\n==========\n" && exit 0
printf "==========\nFailure, simulation should fail\n==========\n"
exit 1
