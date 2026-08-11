# SPDX-License-Identifier: Apache-2.0

yosys -import

read_verilog $::env(TEST_DIR)/dff_worker.v
read_verilog $::env(TEST_DIR)/comb_worker.v
read_verilog $::env(TEST_DIR)/worker.v

yosys hierarchy -check -top worker

yosys proc
yosys rename -wire

yosys synth -flatten
yosys opt

yosys rename -wire

write_json $::env(NETLIST_OUT)
exit
