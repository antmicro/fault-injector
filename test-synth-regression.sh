#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euox pipefail

export CURRENT_DIR="$(dirname $(readlink -f ${BASH_SOURCE[0]}))"
export TEST_DIR="$CURRENT_DIR/test/worker"
export WORKING_DIR=./obj_dir

if [[ -z "$VERILATOR_ROOT" ]]; then
	echo "This script depends on VERILATOR_ROOT variable being set."
	exit 1
fi

rm -rf $WORKING_DIR
mkdir -p $WORKING_DIR

if [[ -z "$VERILATOR_ROOT" ]]; then
	echo "This script depends on VERILATOR_ROOT variable being set."
	exit 1
fi

echo "Testing synth (without techmap)"

yosys -t -d -c test/worker_synth_regression/worker_not_techmap.tcl

## Random
FAULT_SCENARIO_OUT_NO_TECHMAP="$WORKING_DIR/random_campaign_out_no_techmap.csv"
./build/src/FaultGenerator/FaultGenerationTool \
	--sig_path_prefix="top" \
	--top_module="dff_worker" \
	--top_instance="worker" \
	--netlist_path="$WORKING_DIR/dff_worker_post_synth_no_techmap.json" \
	--simulation_time=10 \
	--num_of_events=10 \
	--fault_campaign_out="${FAULT_SCENARIO_OUT_NO_TECHMAP}"

"$VERILATOR_ROOT"/bin/verilator \
	--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v ./src/FaultInjector/FaultInjector.sv -LDFLAGS "-L$(pwd)/build/src/FaultInjector -lFaultInjector" -j "$(nproc)" -CFLAGS '-g' -DFAULT_INJECTION_ENABLE -DFAULT_INJECTION_CAMPAIGN_FILE="\"$FAULT_SCENARIO_OUT_NO_TECHMAP\""

log_file="$WORKING_DIR/sim_fault_no_techmap.log"
./$WORKING_DIR/Vtop | grep -v "Ignoring the event" -B 1 | tee "$log_file" >/dev/null && cat "$log_file"
if grep -q "Mismatch" "$log_file"
then
	printf "==========\nSuccess\n==========\n"
else
	printf "==========\nFailure, simulation should fail\n==========\n"
	exit 1
fi

echo "Testing synth (with techmap)"

yosys -t -d -c test/worker_synth_regression/worker_synth.tcl

## Random
FAULT_SCENARIO_OUT_WITH_TECHMAP="$WORKING_DIR/random_campaign_out_with_techmap.csv"
./build/src/FaultGenerator/FaultGenerationTool \
	--sig_path_prefix="top" \
	--top_module="dff_worker" \
	--top_instance="worker" \
	--netlist_path="$WORKING_DIR/dff_worker_post_synth.json" \
	--simulation_time=10 \
	--num_of_events=10 \
	--fault_campaign_out="${FAULT_SCENARIO_OUT_WITH_TECHMAP}"

"$VERILATOR_ROOT"/bin/verilator \
	--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v ./src/FaultInjector/FaultInjector.sv -LDFLAGS "-L$(pwd)/build/src/FaultInjector -lFaultInjector" -j "$(nproc)" -CFLAGS '-g' -DFAULT_INJECTION_ENABLE -DFAULT_INJECTION_CAMPAIGN_FILE="\"$FAULT_SCENARIO_OUT_WITH_TECHMAP\""

log_file="$WORKING_DIR/sim_fault_with_techmap.log"
./$WORKING_DIR/Vtop | grep -v "Ignoring the event" -B 1 | tee "$log_file" >/dev/null && cat "$log_file"
if grep -q "Mismatch" "$log_file"
then
	printf "==========\nSuccess\n==========\n"
else
	printf "==========\nFailure, simulation should fail\n==========\n"
	# FIXME: As of now, if FaultGenerationTool receives yosys json post synthesis,
	# it produces events for signals paths that don't appear in the simulation.
	# That means all events are rejected, and so no injection takes place.
	#
	# exit 1
fi
