# Faultergeist
Copyright (c) 2026 Antmicro

A fault injection framework for hardware design simulation, including a plugin
for VPI-compatible simulators. Generates fault campaigns based on a netlist and
injects them into a simulation via VPI.

## Setup
To build the project, run:

```sh
./build.sh
```

To build and run unit tests:

```sh
./build.sh -DBUILD_TESTING=ON
ctest --test-dir build
```

## Usage
To generate and simulate a fault campaign:

1. Run synthesis to get a JSON netlist file with mapped DFFs.

```
yosys <<EOF
	read_verilog [... SV sources ...]
	proc
	rename -wire
	write_json netlist.json
	exit
EOF
```

2. Emit faults.

```bash
faultergeist-gen \
  --sig_path_prefix="top" \
  --top_module="instance" \
  --top_instance="instance_name" \
  --netlist_path="netlist.json" \
  --fault_campaign_out="fault_campaign_out.csv"
```

3. Initialize FI module in test bench code.
```verilog
module top;
...
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi("fault_campaign_out.csv");
`endif
...
endmodule
```

4. Invoke verilation with FaultInjection sources and link with library.

```bash
verilator \
    [... verilator flags ...] \
    --vpi --public-flat-rw \ # Enable VPI and expose signals for injection
    [... SV sources ...] \
    faultergeist-inject.sv \ # Include FI library module
    -LDFLAGS "-L$PATH_TO_FI_LIB_DIR -lfaultergeist-inject" \ # Link with FI library
    -DFAULT_INJECTION_ENABLE # Enable FI
```
