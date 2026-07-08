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

cmake -B build \
	-DVERILATOR_ROOT="$VERILATOR_ROOT" \
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

FAULT_SCENARIOS_DIR="$WORKING_DIR/fault_campaign_out"

./build/src/FaultGenerator/FaultGenerationTool \
	--sig_path_prefix="top" \
	--top_module="dff_worker" \
	--top_instance="worker" \
	--netlist_path="$WORKING_DIR/worker.json" \
	--simulation_time=10 \
	--num_of_events=10 \
	--fault_campaign_out="$FAULT_SCENARIOS_DIR" \
	--campaign_number=10 \
	--thread_number=`nproc`

"$VERILATOR_ROOT"/bin/verilator \
	--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v -j "$(nproc)" -CFLAGS '-g'

# TODO go back to normal printing when excessive logs (from unrecognized signal) are fixed
# Print logs, only when simulation failed
$WORKING_DIR/Vtop | tee $WORKING_DIR/sim_no_fault.log >/dev/null || cat $WORKING_DIR/sim_no_fault.log
grep -q "Mismatch" $WORKING_DIR/sim_no_fault.log && printf "==========\nFailure, simulation should pass\n==========\n" && exit 1

total=0
failed=0
for CAMPAIGN in `find $FAULT_SCENARIOS_DIR -type f`
do
	total=$((total+1))
	echo "-------------------------------------------------------------------------------"
	echo "Doing campaign [#$total]: $CAMPAIGN"
	log_file="$WORKING_DIR/sim_fault/run#$total.log"
	mkdir -p "$(dirname "$log_file")"

	"$VERILATOR_ROOT"/bin/verilator \
		--binary --vpi $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v ./src/FaultInjector/FaultInjector.sv -LDFLAGS "-L$(pwd)/build/src/FaultInjector -lFaultInjector" -j "$(nproc)" -CFLAGS '-g' -DFAULT_INJECTION_ENABLE -DFAULT_INJECTION_CAMPAIGN_FILE="\"$CAMPAIGN\""

	# Print logs, only when simulation succeeded (when it should have failed)
	./$WORKING_DIR/Vtop | tee "$log_file" >/dev/null && cat "$log_file"
	if grep -q "Mismatch" "$log_file"
	then
		printf "==========\nSuccess\n==========\n"
	else
		failed=$((failed+1))
		printf "==========\nFailure, simulation should fail\n==========\n"
	fi
done

echo "==============================================================================="
echo "Result [$((total-failed))/$total]"
exit $failed

