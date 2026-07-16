# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED FI_COMMAND)
  message(FATAL_ERROR "FI_COMMAND is required")
endif()
if("${FI_COMMAND}" STREQUAL "")
  message(FATAL_ERROR "FI_COMMAND is empty")
endif()
if(NOT DEFINED FI_LOG)
  message(FATAL_ERROR "FI_LOG is required")
endif()
if(NOT DEFINED FI_WORK_DIR)
  set(FI_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

list(GET FI_COMMAND 0 _fi_executable)
if("${_fi_executable}" STREQUAL "")
  message(FATAL_ERROR "FI_COMMAND has an empty executable: ${FI_COMMAND}")
endif()
if(NOT EXISTS "${_fi_executable}")
  message(FATAL_ERROR "Simulation executable does not exist: ${_fi_executable}\nFull command: ${FI_COMMAND}\nWorking directory: ${FI_WORK_DIR}")
endif()
if(IS_DIRECTORY "${_fi_executable}")
  message(FATAL_ERROR "Simulation executable is a directory: ${_fi_executable}\nFull command: ${FI_COMMAND}\nWorking directory: ${FI_WORK_DIR}")
endif()

get_filename_component(_log_dir "${FI_LOG}" DIRECTORY)
file(MAKE_DIRECTORY "${_log_dir}")

execute_process(
  COMMAND ${FI_COMMAND}
  WORKING_DIRECTORY "${FI_WORK_DIR}"
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

set(_combined "${_stdout}${_stderr}")
file(WRITE "${FI_LOG}" "${_combined}")

if(NOT _result EQUAL 0)
  message(FATAL_ERROR "Simulation command failed with exit code ${_result}. See ${FI_LOG}")
endif()

string(FIND "${_combined}" "Mismatch" _mismatch_pos)

if(FI_EXPECT_VCD_GOLDENFILE)
  list(GET FI_EXPECT_VCD_GOLDENFILE 0 _vcd_actual)
  list(GET FI_EXPECT_VCD_GOLDENFILE 1 _vcd_expected)

  if(NOT EXISTS "${_vcd_actual}")
    message(FATAL_ERROR "Golden file assertion failed. Actual VCD file does not exist: ${_vcd_actual}")
  endif()
  if(NOT EXISTS "${_vcd_expected}")
    message(FATAL_ERROR "Golden file assertion failed. Expected VCD file does not exist: ${_vcd_expected}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files ${_vcd_expected} ${_vcd_actual}
    RESULT_VARIABLE _expect_vcd_result
  )

  if(NOT _expect_vcd_result EQUAL 0)
    message(FATAL_ERROR "Golden file assertion failed. Expected ${_vcd_expected} and ${_vcd_actual} are not equal.")
  endif()
elseif(FI_EXPECT_MISMATCH)
  if(_mismatch_pos EQUAL -1)
    message(FATAL_ERROR "Simulation was expected to report Mismatch, but did not. See ${FI_LOG}")
  endif()
else()
  if(NOT _mismatch_pos EQUAL -1)
    message(FATAL_ERROR "Simulation reported unexpected Mismatch. See ${FI_LOG}")
  endif()
endif()
