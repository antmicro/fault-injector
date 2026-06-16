# Fault injector
Fault injector is a collection of tools meant to use as a plugin to Verilator,
to allow for testing circuit's design for it's handling of single event errors
[wiki](https://en.wikipedia.org/wiki/Single-event_upset).

## Setup
To build project it's enough to call `build.sh`.

To build unit tests, call `build.sh -DBUILD_TESTING=ON`.
Then to run them call `ctest --test-dir build`.

To run end-to-end tests use `bash test.sh`.

## Usage
To generate fault campaign `fault_campaign_out.csv` to be used by FaultInjection library:

1. Run synthesis to get JSON netlist file with mapped DFFs.

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
