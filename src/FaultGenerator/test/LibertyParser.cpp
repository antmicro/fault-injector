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

#include "LibertyParser.h"
#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>

ABSL_FLAG(std::string, input_path, "", "[REQUIRED] input file");

int main(int argc, char** argv) {
    absl::ParseCommandLine(argc, argv);

    if (absl::GetFlag(FLAGS_input_path).empty()) {
        std::cerr << "Missing \"--input_path\" flag\n";
        std::exit(1);
    }

    auto parse_result = LibertyParser::parse(absl::GetFlag(FLAGS_input_path));
    if (!parse_result) {
        std::cerr << "Failed to parse\n";
        return 1;
    }

    printf("library\n");
    for (const auto& [cell_name, cell_info] : parse_result->cells) {
        printf("    cell(\"%s\") { area : %lf; }\n", cell_name.c_str(), cell_info.area);
    }
}
