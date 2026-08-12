// Copyright 2026 Antmicro <antmicro.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

parameter int CYCLES = 20;
parameter int CLOCK_PERIOD = 5;

`ifndef VCD_OUTPUT_PATH
  `error "VCD_OUTPUT_PATH must be defined during Verilator compilation"
`endif

module top;
  logic clk = 0;
  initial forever #CLOCK_PERIOD clk = !clk;

  reg req_vld = 1;
  reg resp_rdy = 1;
  reg [31:0] req = 32'h0000BEEF;
  wire req0_rdy;
  wire req1_rdy;
  wire resp0_vld;
  wire resp1_vld;
  wire [31:0] resp0;
  wire [31:0] resp1;
  wire [31:0] counter;

`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi(`FAULT_INJECTION_CAMPAIGN_FILE);

  always @(counter) begin
    if (counter != 32'(($time + 64'(CLOCK_PERIOD)) / 64'(2 * CLOCK_PERIOD))) begin
      $display("[%0t] Mismatch transient counter=%d", $time, counter);
    end
  end
`endif

  worker worker (.*);

  final begin
    if (counter * CLOCK_PERIOD * 2 != CYCLES) begin
      $display("[%0t] Mismatch %d != %d", $time, CYCLES, counter * CLOCK_PERIOD * 2);
    end
  end

  initial begin
    if ($test$plusargs("trace") != 0) begin
      $display("[%0t] Tracing to %s...\n", $time, `VCD_OUTPUT_PATH);
      $dumpfile(`VCD_OUTPUT_PATH);
      $dumpvars();
      $display("[%0t] Model running...\n", $time);
    end

    #CYCLES $finish;
  end

endmodule
