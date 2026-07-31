# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED FI_INPUT)
  message(FATAL_ERROR "FI_INPUT is required")
endif()

if(NOT DEFINED FI_OUTPUT)
  message(FATAL_ERROR "FI_OUTPUT is required")
endif()

configure_file("${FI_INPUT}" "${FI_OUTPUT}" @ONLY)
