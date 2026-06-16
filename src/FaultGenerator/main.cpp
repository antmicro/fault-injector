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

#include "FaultEvent.h"
#include "GlobalOpts.h"
#include "Signal.h"
#include "SignalCollector.h"

#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
    const GlobalOpts opts = GlobalOpts::parse_cmd_args(argc, argv);

    std::vector<Signal> signals =
        SignalCollector(opts.top_module, opts.top_instance, opts.sig_path_prefix)
            .collectFromFile(opts.netlist_path);

    const std::vector<FaultEvent> fault_events = opts.strategy->generate(signals);
    std::ofstream of(opts.fault_campaign_out);
    for (const FaultEvent& fault_event : fault_events) {
        of << fault_event.time << ',' << fault_event.signal_path << ',' << fault_event.bit_index
           << ',' << faultEventTypeToString(fault_event.type) << '\n';
    }
}
