#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

CURRENT_DIR="$(dirname $(readlink -f ${BASH_SOURCE[0]}))"
TEST_DIR="$CURRENT_DIR/test/worker"
WORKING_DIR=./obj_dir

if [[ -z "$VERILATOR_ROOT" ]]
then
  echo "This script depends on VERILATOR_ROOT variable being set."
  exit 1
fi

rm -rf $WORKING_DIR
mkdir -p $WORKING_DIR

if [[ -z "$VERILATOR_ROOT" ]]
then
  echo "This script depends on VERILATOR_ROOT variable being set."
  exit 1
fi

cmake -B build \
	-DBUILD_TESTING=OFF \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DCMAKE_BUILD_TYPE=Debug

cmake --build build -j

yosys <<EOF
	read_verilog $TEST_DIR/dff_worker.v
	proc
	rename -wire
	write_json $WORKING_DIR/worker.json
	exit
EOF

FAULT_SCENARIO_OUT="$WORKING_DIR/fault_campaign_out.csv"

./build/src/FaultGenerator/FaultGenerationTool \
	--sig_path_prefix="top" \
	--top_module="dff_worker" \
	--top_instance="worker" \
	--netlist_path="$WORKING_DIR/worker.json" \
	--simulation_time=10 \
	--num_of_events=10 \
  --fault_campaign_out="$FAULT_SCENARIO_OUT"

"$VERILATOR_ROOT"/bin/verilator \
	--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v -j "$(nproc)" -CFLAGS '-g'

# TODO go back to normal printing when excessive logs (from unrecognized signal) are fixed
# Print logs, only when simulation failed
log_file="$WORKING_DIR/sim_no_fault.log"
./$WORKING_DIR/Vtop | grep -v "Ignoring the event" -B 1 | tee "$log_file" >/dev/null || cat "$log_file"
grep -q "Mismatch" "$log_file" && printf "==========\nFailure, simulation should pass\n==========\n" && exit 1

"$VERILATOR_ROOT"/bin/verilator \
	--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v ./src/FaultInjector/FaultInjector.sv -LDFLAGS "-L$(pwd)/build/src/FaultInjector -lFaultInjector" -j "$(nproc)" -CFLAGS '-g' -DFAULT_INJECTION_ENABLE -DFAULT_INJECTION_CAMPAIGN_FILE="\"$FAULT_SCENARIO_OUT\""

# Print logs, only when simulation succeeded (when it should have failed)
log_file="$WORKING_DIR/sim_fault.log"
./$WORKING_DIR/Vtop | grep -v "Ignoring the event" -B 1 | tee "$log_file" >/dev/null && cat "$log_file"
grep -q "Mismatch" "$log_file" && printf "==========\nSuccess\n==========\n" && exit 0
printf "==========\nFailure, simulation should fail\n==========\n"
exit 1
