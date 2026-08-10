#!/bin/bash

set -euox pipefail

VERILATOR=""
if [[ -z "$VERILATOR_ROOT" ]]; then
  VERILATOR="$(which verilator)"
else
  VERILATOR="$VERILATOR_ROOT/bin/verilator"
fi

cat <<EOF
# ==============================================================================
# The following instructions demonstrate FaultGenerator tool and FaultInjector
# plugin using "worker" core as an example.
# ==============================================================================
EOF

# It should be run in the directory it is in
cd "$(dirname "$0")"

cat <<EOF
# ==============================================================================
# Before anything the project must be configured and built.
# ==============================================================================
EOF
cmake -B build -S .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build build -j "$(nproc)"

cat <<EOF
# ==============================================================================
# Next step is to generate list of signals to injection. We can do this using
# yosys.
# ==============================================================================
EOF
yosys <<EOF
  read_verilog worker/dff_worker.v
  proc
  rename -wire
  write_json netlist.json
  exit
EOF


cat <<EOF
# ==============================================================================
# Once we have the netlist, we can proceed to execute FaultGenerationTool, to
# generate fault campaign To do so we pass it a config file like one prepared
# in this directory. This is where we select model to use, but also other
# parameters for the generation.
# ==============================================================================
EOF
build/src/FaultGenerator/FaultGenerationTool --config_file=config.json.in

cat <<EOF
# ==============================================================================
# Next step is to prepare the simulation code. To have faults injected during
# the simulation, we must pass verilator couple extra arguments.
# To explain the call:
# - '--binary', '-j $(nproc)' are normal flags you would pass to verilator. They
#   cause verilator to generate all simulation files and build them into a 
#   single binary.
# - '--vpi' '--public-flat-rw' - these are flags which enable verilator features
#   we rely on to inject faults
# - worker/... - these are system verilog sources, these should be replaced by
#   ones in your design
# - FaultInjector.sv - this is system verilog side of fault injection library
# - '-LDFLAGS "-Lbuild/src/FaultInjector"' '-lFaultInjector' - this is c++ side
#   of fault injection library
# - '-DFAULT_INJECTION_ENABLE' - this flag enables fault injection in the
#   simulation
# - -DFAULT_INJECTION_CAMPAIGN_FILE="\"fault_campaign_out.csv\"" - this points
#   verilator at where is the file containing the campaign
# ==============================================================================
EOF
$VERILATOR \
    --binary -j "$(nproc)" \
    --vpi --public-flat-rw \
    worker/config.vlt worker/top.v worker/worker.v worker/comb_worker.v worker/dff_worker.v \
    ../src/FaultInjector/FaultInjector.sv \
    -LDFLAGS "-L$(pwd)/build/src/FaultInjector -lFaultInjector" \
    -DFAULT_INJECTION_ENABLE \
    -DFAULT_INJECTION_CAMPAIGN_FILE="\"fault_campaign_out.csv\""

cat <<EOF
# ==============================================================================
# Last thing left to run the simulation and watch how faults impact the design.
# ==============================================================================
EOF
obj_dir/Vtop
