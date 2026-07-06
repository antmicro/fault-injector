#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

CURRENT_DIR="$(dirname $(readlink -f ${BASH_SOURCE[0]}))"
TEST_DIR="$CURRENT_DIR/worker"

if [[ -z "$VERILATOR_ROOT" ]]
then
  echo "This script depends on VERILATOR_ROOT variable being set."
  exit 1
fi

pushd "$CURRENT_DIR"
rm -rf ./obj_dir

# TODO run verilator once, but change define to take different DFAULT_INJECTION_CAMPAIGN_FILE and avoid
# excessive verilation

"$VERILATOR_ROOT"/bin/verilator \
	--trace --trace-vcd --exe --cc --vpi --timing --build $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v -j "$(nproc)" -CFLAGS '-g' $CURRENT_DIR/TestWorker.cpp

./obj_dir/Vtop
mv logs ./obj_dir/logs-baseline
diff ./obj_dir/logs-baseline/vlt_dump.vcd baseline_dump.vcd.out

"$VERILATOR_ROOT"/bin/verilator \
	--trace --trace-vcd --exe --cc --vpi --timing --build $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v $CURRENT_DIR/../FaultInjector.sv -LDFLAGS "-L$CURRENT_DIR/../../../build/src/FaultInjector -lFaultInjector" -j "$(nproc)" -CFLAGS '-g' -DFAULT_INJECTION_ENABLE $CURRENT_DIR/TestWorker.cpp -DFAULT_INJECTION_CAMPAIGN_FILE="\"fault_injector_set_test.csv.in\""

./obj_dir/Vtop
mv logs ./obj_dir/logs-set
diff ./obj_dir/logs-set/vlt_dump.vcd set_dump.vcd.out

"$VERILATOR_ROOT"/bin/verilator \
	--trace --trace-vcd --exe --cc --vpi --timing --build $TEST_DIR/config.vlt $TEST_DIR/top.v $TEST_DIR/worker.v $TEST_DIR/comb_worker.v $TEST_DIR/dff_worker.v $CURRENT_DIR/../FaultInjector.sv -LDFLAGS "-L$CURRENT_DIR/../../../build/src/FaultInjector -lFaultInjector" -j "$(nproc)" -CFLAGS '-g' -DFAULT_INJECTION_ENABLE $CURRENT_DIR/TestWorker.cpp -DFAULT_INJECTION_CAMPAIGN_FILE="\"fault_injector_seu_test.csv.in\""

./obj_dir/Vtop
mv logs ./obj_dir/logs-seu
diff ./obj_dir/logs-seu/vlt_dump.vcd seu_dump.vcd.out

popd
