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

module worker (
    input clk,
    input wire req_vld,
    input wire resp_rdy,
    input wire [31:0] req,
    output reg req0_rdy,
    output reg resp0_vld,
    output reg [31:0] resp0,
    output reg req1_rdy,
    output reg resp1_vld,
    output reg [31:0] resp1,
    output reg [31:0] counter
);
  comb_worker comb_worker0 (/*AUTOINST*/
                            // Inputs
                            .clk      (clk),
                            .req_vld  (req_vld),
                            .resp_rdy (resp_rdy),
                            .req      (req),
                            // Outputs
                            .req_rdy  (req0_rdy),
                            .resp_vld (resp0_vld),
                            .resp     (resp0));
  dff_worker dff_worker0 (/*AUTOINST*/
                          // Inputs
                          .clk      (clk),
                          .req_vld  (req_vld),
                          .resp_rdy (resp_rdy),
                          .req      (req),
                          // Outputs
                          .req_rdy  (req1_rdy),
                          .resp_vld (resp1_vld),
                          .resp     (resp1),
                          .counter  (counter));
endmodule
