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

module comb_worker (
    input clk,
    input wire req_vld,
    input wire resp_rdy,
    input wire [31:0] req,
    output wire req_rdy,
    output wire resp_vld,
    output wire [31:0] resp
);
  assign req_rdy = req_vld;
  assign resp_vld = req_vld;
  assign resp = req_vld ? req : 32'h0000000E;
endmodule
