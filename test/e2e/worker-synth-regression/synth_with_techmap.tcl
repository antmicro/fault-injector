# SPDX-License-Identifier: Apache-2.0

yosys -import
read_verilog $::env(TEST_DIR)/dff_worker.v
synth
yosys rename -wire
log "Writing json"
write_json $::env(NETLIST_OUT)
exit
