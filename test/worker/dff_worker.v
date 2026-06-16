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

module dff_worker (
    input clk,
    input wire req_vld,
    input wire resp_rdy,
    input wire [31:0] req,
    output reg req_rdy,
    output reg resp_vld,
    output reg [31:0] resp,
    output reg [31:0] counter
);
  assign req_rdy = req_vld;
  assign resp_vld = req_vld;

  always @(posedge clk) begin
    if (req_vld) begin
      resp <= req;
      counter <= counter + 1;
    end else begin
      resp <= 0;
    end
  end
endmodule
