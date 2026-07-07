yosys -import
plugin -i slang
yosys read_slang --top $::env(DESIGN_TOP) --std latest --single-unit --allow-toplevel-iface-ports -F $::env(TEST_DIR)/$::env(DESIGN_FILE_LIST)
# TODO: replace with fine-grained synthesis
synth -top $::env(DESIGN_TOP)
clean
write_verilog $::env(TEST_DIR)/$::env(DESIGN_TOP)_post_synth.v
