# SPDX-License-Identifier: Apache-2.0

yosys -import
read_verilog $::env(TEST_DIR)/dff_worker.v
yosys proc
yosys rename -wire
write_json $::env(NETLIST_OUT)
exit
