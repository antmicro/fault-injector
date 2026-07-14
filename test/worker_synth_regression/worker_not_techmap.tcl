yosys -import
read_verilog $::env(TEST_DIR)/dff_worker.v
# The following corresponds to 'synth' command:
hierarchy -check
yosys proc
check
flatten
opt_expr
opt_clean
check
opt -nodffe -nosdff
fsm
opt
wreduce
peepopt
opt_clean
alumacc
share
opt
memory -nomap
opt_clean
opt -fast -full
memory_map
opt -full
opt -fast
abc
opt -fast
hierarchy -check
stat
check
# Synthesis done, write output
yosys rename -wire
log "Writing json"
write_json $::env(WORKING_DIR)/dff_worker_post_synth_no_techmap.json
exit

