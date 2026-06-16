#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cmake -B build \
  -DVERILATOR_ROOT="$VERILATOR_ROOT" \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build -j
