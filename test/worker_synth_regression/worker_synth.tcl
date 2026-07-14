yosys -import
read_verilog $::env(TEST_DIR)/dff_worker.v
synth
yosys rename -wire
log "Writing json"
write_json $::env(WORKING_DIR)/dff_worker_post_synth.json
exit

