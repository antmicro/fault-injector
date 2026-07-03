# Fault injector
Copyright (c) 2026 Antmicro

A collection of tools, including a plugin for Verilator,
that allow for injecting faults into a hardware design simulation.

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

To run end-to-end tests:

```sh
./bash test.sh
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
FaultGenerationTool \
  --sig_path_prefix="top" \
  --top_module="instance" \
  --top_instance="instance_name" \
  --netlist="netlist.json" \
  --fault_campaign_out="fault_campaign_out.csv"
```

3. Initialize FI module in test bench code.
```verilog
module top;
...
`ifdef FAULT_INJECTION_ENABLE
  FaultInjector fi("fault_campaign_out.csv");
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
    FaultInjector.sv \ # Include FI library module
    -LDFLAGS "-L$PATH_TO_FI_LIB_DIR -lFaultInjector" \ # Link with FI library
    -DFAULT_INJECTION_ENABLE # Enable FI
```
