// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: main() simulation loop, created with --main

#include <memory>
#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "verilated_vpi.h"

// User VPI code adds to this array to get startup main() callbacks
extern "C" void (*vlog_startup_routines[])() VL_ATTR_WEAK;

//======================

int main(int argc, char** argv, char**) {
    // Setup context, defaults, and parse command line
    Verilated::debug(0);
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->traceEverOn(true);
    contextp->threads(1);
    contextp->commandArgs(argc, argv);

    // Construct the Verilated model, from Vtop.h generated from Verilating
    const std::unique_ptr<Vtop> topp{new Vtop{contextp.get(), ""}};

    // Hook VPI startup routines and invoke callback
    if (vlog_startup_routines) {
        for (auto routinep = &vlog_startup_routines[0]; *routinep; routinep++) {
            (*routinep)();
        }
    }
    VerilatedVpi::callCbs(cbStartOfSimulation);

    auto tfp = std::make_unique<VerilatedVcdC>();
    topp->trace(tfp.get(), 99);  // Trace 99 levels of hierarchy
    Verilated::mkdir("logs");
    tfp->open("logs/vlt_dump.vcd");

    // Simulate until $finish
    while (VL_LIKELY(!contextp->gotFinish())) {
        // VPI callbacks
        VerilatedVpi::callTimedCbs();
        VerilatedVpi::callCbs(cbNextSimTime);
        VerilatedVpi::callCbs(cbAtStartOfSimTime);
        // Evaluate model
        topp->eval();
        if (tfp) {
            tfp->dump(contextp->time());
        }
        // VPI callbacks
        VerilatedVpi::callValueCbs();
        VerilatedVpi::callCbs(cbAtEndOfSimTime);
        VerilatedVpi::callCbs(cbReadWriteSynch);
        VerilatedVpi::callCbs(cbReadOnlySynch);
        // Advance time
        if (!topp->eventsPending()) {
            break;
        }
        contextp->time(std::min(topp->nextTimeSlot(), VerilatedVpi::cbNextDeadline()));
    }

    if (VL_LIKELY(!contextp->gotFinish())) {
        VL_DEBUG_IF(VL_PRINTF("+ Exiting without $finish; no events left\n"););
    }

    // Execute 'final' processes
    topp->final();
    VerilatedVpi::callCbs(cbEndOfSimulation);

    // Print statistical summary report
    contextp->statsPrintSummary();

    return 0;
}
