# SPDX-License-Identifier: Apache-2.0

include(ProcessorCount)

set(FI_E2E_SCRIPT_DIR "${PROJECT_SOURCE_DIR}/test/cmake")
set(FI_E2E_WORKER_DIR "${PROJECT_SOURCE_DIR}/test/worker")
set(FI_E2E_FAULT_INJECTOR_DIR "${PROJECT_SOURCE_DIR}/src/FaultInjector")
set(FI_E2E_TOOLS_TARGET fi-e2e-tools)
set(FI_E2E_SKIP_CODE 77)

set(WORKER_SOURCES
  "${FI_E2E_WORKER_DIR}/config.vlt"
  "${FI_E2E_WORKER_DIR}/top.v"
  "${FI_E2E_WORKER_DIR}/worker.v"
  "${FI_E2E_WORKER_DIR}/comb_worker.v"
  "${FI_E2E_WORKER_DIR}/dff_worker.v"
)

# Argument/default conventions used by the helpers below:
#
# - Parsed keyword arguments use the FI_ prefix produced by
#   cmake_parse_arguments(). For example, OUTPUT becomes FI_OUTPUT.
# - _fi_default_arg(FI_ARG ARG) resolves scalar arguments in this order:
#     1. Explicit parsed argument, e.g. FI_ARG.
#     2. Variable in the caller directory/function scope, e.g. ARG.
#     3. Directory property definition for ARG.
#   It errors if none of those sources provides a non-empty value.
# - _fi_resolve_path_default(NAME SUBDIRECTORY FALLBACK_NAME) resolves path
#   arguments in this order:
#     1. Explicit parsed argument FI_${NAME}.
#     2. Variable ${NAME}.
#     3. Directory property definition for ${NAME}.
#     4. ${FALLBACK_NAME}/${SUBDIRECTORY}.
#   When fallback is used inside a helper, it also sets ${NAME} in that helper's
#   scope so later resolver calls in the same helper observe the same path.
#
# Common variables expected by most tests:
#   WORK_DIR         Build-tree working root for the scenario.
#   SIMULATION_DIR  Per-run output directory. Defaults to ${WORK_DIR}/obj_dir.
#   SOURCES         Verilog source list for Yosys netlist generation.
#   WORKER_SOURCES  Verilator source list for worker simulations.
#   TEST_DIR        Source fixture directory exported to Yosys scripts.
#   NETLIST_PATH    Yosys JSON netlist output.
#   FAULT_CAMPAIGN_OUT
#                   FaultGenerationTool output. Defaults to
#                   ${SIMULATION_DIR}/fault_campaign_out.csv.

macro(_fi_default_arg ARG_NAME FALLBACK_NAME)
  set(_fi_default_value "")
  if(DEFINED ${ARG_NAME} AND NOT "${${ARG_NAME}}" STREQUAL "")
  elseif(DEFINED ${FALLBACK_NAME} AND NOT "${${FALLBACK_NAME}}" STREQUAL "")
    set(${ARG_NAME} "${${FALLBACK_NAME}}")
  else()
    get_directory_property(_fi_default_value DEFINITION ${FALLBACK_NAME})
    if(NOT "${_fi_default_value}" STREQUAL "")
      set(${ARG_NAME} "${_fi_default_value}")
    else()
      message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} requires ${ARG_NAME} or ${FALLBACK_NAME}")
    endif()
  endif()
endmacro()

macro(_fi_resolve_path_default NAME SUBDIRECTORY FALLBACK_NAME)
  set(_fi_path_arg "FI_${NAME}")
  set(_fi_path_value "")
  set(_fi_fallback_value "")
  if(DEFINED ${_fi_path_arg} AND NOT "${${_fi_path_arg}}" STREQUAL "")
  elseif(DEFINED ${NAME} AND NOT "${${NAME}}" STREQUAL "")
    set(${_fi_path_arg} "${${NAME}}")
  else()
    get_directory_property(_fi_path_value DEFINITION ${NAME})
    if(NOT "${_fi_path_value}" STREQUAL "")
      set(${_fi_path_arg} "${_fi_path_value}")
    else()
      if(DEFINED ${FALLBACK_NAME} AND NOT "${${FALLBACK_NAME}}" STREQUAL "")
        set(_fi_fallback_value "${${FALLBACK_NAME}}")
      else()
        get_directory_property(_fi_fallback_value DEFINITION ${FALLBACK_NAME})
      endif()

      if(NOT "${_fi_fallback_value}" STREQUAL "")
        if("${SUBDIRECTORY}" STREQUAL "")
          set(${NAME} "${_fi_fallback_value}")
        else()
          set(${NAME} "${_fi_fallback_value}/${SUBDIRECTORY}")
        endif()
        set(${_fi_path_arg} "${${NAME}}")
      else()
        message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} requires ${_fi_path_arg}, ${NAME}, or ${FALLBACK_NAME}")
      endif()
    endif()
  endif()
endmacro()

function(fi_require_e2e_tools)
  if(DEFINED VERILATOR_ROOT AND NOT VERILATOR_ROOT STREQUAL "")
    set(_verilator "${VERILATOR_ROOT}/bin/verilator")
    set(VERILATOR_EXECUTABLE "${_verilator}" CACHE FILEPATH "Path to verilator" FORCE)
  elseif(DEFINED ENV{VERILATOR_ROOT} AND NOT "$ENV{VERILATOR_ROOT}" STREQUAL "")
    set(_verilator "$ENV{VERILATOR_ROOT}/bin/verilator")
    set(VERILATOR_EXECUTABLE "${_verilator}" CACHE FILEPATH "Path to verilator" FORCE)
  else()
    find_program(VERILATOR_EXECUTABLE verilator)
  endif()

  find_program(YOSYS_EXECUTABLE yosys)

  ProcessorCount(_processor_count)
  if(_processor_count EQUAL 0)
    set(_processor_count 1)
  endif()
  set(FI_E2E_JOBS "${_processor_count}" CACHE STRING "Parallel jobs used by E2E Verilator/FaultGenerationTool commands")

  set(FI_E2E_TOOLS_FOUND TRUE)
  foreach(_tool VERILATOR_EXECUTABLE YOSYS_EXECUTABLE)
    if(NOT ${_tool} OR "${${_tool}}" MATCHES "-NOTFOUND$")
      set(FI_E2E_TOOLS_FOUND FALSE)
    endif()
  endforeach()

  if(NOT FI_E2E_TOOLS_FOUND)
    message(WARNING "E2E tools (Verilator/Yosys) not found. E2E tests will be skipped.")

    if(NOT TARGET ${FI_E2E_TOOLS_TARGET})
      add_custom_target(${FI_E2E_TOOLS_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Error: E2E tools were not found during configuration."
        COMMAND ${CMAKE_COMMAND} -E false
      )
    endif()
  else()
    if(NOT TARGET ${FI_E2E_TOOLS_TARGET})
        add_custom_target(${FI_E2E_TOOLS_TARGET})
    endif()
  endif()
endfunction()

function(fi_add_ctest_scenario NAME TARGET_NAME)
  set(options)
  set(one_value_args)
  set(multi_value_args DEPENDS)
  cmake_parse_arguments("FI" "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  add_custom_target(${TARGET_NAME}
    DEPENDS ${FI_DEPENDS}
  )

  add_test(
    NAME "${NAME}"
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target "${TARGET_NAME}" --config $<CONFIG>
  )
  set_tests_properties("${NAME}" PROPERTIES
      LABELS "e2e;yosys;verilator"
      SKIP_RETURN_CODE ${FI_E2E_SKIP_CODE}
  )
endfunction()

# fi_add_yosys_json(<target>
#   [SCRIPT <path>] [OUTPUT <path>] [WORK_DIR <dir>]
#   [TEST_DIR <dir>] [SOURCES <files...>] [ENV <VAR=value>...])
#
# Required, explicit or implicit:
#   SCRIPT             Yosys Tcl script. May come from variable SCRIPT.
#   OUTPUT             JSON netlist. May come from NETLIST_PATH.
#   WORK_DIR           Build working directory.
#   SOURCES            Yosys input sources. May come from SOURCES.
#   TEST_DIR           Fixture directory exported to the script. May come from
#                      TEST_DIR.
#
# Build-time environment passed to Yosys:
#   NETLIST_OUT=<OUTPUT>
#   TEST_DIR=<TEST_DIR>
#   SOURCES=<SOURCES>
#   plus any values from ENV.
function(fi_add_yosys_json NAME)
  set(options)
  set(one_value_args SCRIPT OUTPUT WORK_DIR TEST_DIR)
  set(multi_value_args SOURCES ENV)
  cmake_parse_arguments("FI" "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_default_arg(FI_SCRIPT SCRIPT)
  _fi_default_arg(FI_OUTPUT NETLIST_PATH)
  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  _fi_default_arg(FI_SOURCES SOURCES)
  _fi_default_arg(FI_TEST_DIR TEST_DIR)

  get_filename_component(_output_dir "${FI_OUTPUT}" DIRECTORY)

  add_custom_command(
    OUTPUT "${FI_OUTPUT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_output_dir}" "${FI_WORK_DIR}"
    COMMAND
      "${CMAKE_COMMAND}" -E env
      "NETLIST_OUT=${FI_OUTPUT}"
      "TEST_DIR=${FI_TEST_DIR}"
      "SOURCES=${FI_SOURCES}"
      ${FI_ENV}
      "${YOSYS_EXECUTABLE}" -c "${FI_SCRIPT}"
    DEPENDS "${FI_SCRIPT}" ${FI_SOURCES}
    VERBATIM
  )
  add_custom_target("${NAME}" DEPENDS "${FI_OUTPUT}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
endfunction()

# fi_configure_file(<input> [OUTPUT <path>] [SIMULATION_DIR <dir>])
#
# Configures <input> with @ONLY substitution.
#
# Required:
#   input              Template file.
#
# Optional/implicit:
#   OUTPUT            Configured output file. Defaults to
#                     ${SIMULATION_DIR}/config.json.
#   SIMULATION_DIR    May be explicit, come from SIMULATION_DIR, or default to
#                     ${WORK_DIR}/obj_dir.
function(fi_configure_file INPUT)
  set(options)
  set(one_value_args OUTPUT SIMULATION_DIR)
  set(multi_value_args)
  cmake_parse_arguments(FI "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_resolve_path_default(SIMULATION_DIR obj_dir WORK_DIR)

  if(NOT FI_OUTPUT)
    set(FI_OUTPUT "${FI_SIMULATION_DIR}/config.json")
  endif()

  get_filename_component(_output_dir "${FI_OUTPUT}" DIRECTORY)
  file(MAKE_DIRECTORY "${_output_dir}")
  configure_file("${INPUT}" "${FI_OUTPUT}" @ONLY)
endfunction()

# fi_add_fault_campaign(<target>
#   [OUTPUT <path>] [CONFIG <path> | CONFIG_FILE <path>]
#   [WORK_DIR <dir>] [SIMULATION_DIR <dir>]
#   [ARGS <args...>] [DEPENDS <deps...>])
#
# Generates a fault campaign with FaultGenerationTool.
#
# Required, explicit or implicit:
#   WORK_DIR          Command working directory.
#   SIMULATION_DIR   Per-run output directory. Defaults to ${WORK_DIR}/obj_dir.
#
# Optional/implicit:
#   OUTPUT            CMake-tracked output. Defaults to FAULT_CAMPAIGN_OUT.
#   FAULT_CAMPAIGN_OUT
#                     Tool output path. Defaults to
#                     ${SIMULATION_DIR}/fault_campaign_out.csv.
#   CONFIG/CONFIG_FILE
#                     Config file passed as --config_file. If neither CONFIG nor
#                     ARGS is supplied, defaults to
#                     ${SIMULATION_DIR}/config.json.
#   ARGS              Direct FaultGenerationTool arguments. When ARGS is
#                     non-empty, no implicit config file is added.
#
# On rerun, the helper removes OUTPUT and FAULT_CAMPAIGN_OUT before invoking the
# tool, then recreates the required directories.
function(fi_add_fault_campaign NAME)
  set(options)
  set(one_value_args OUTPUT CONFIG CONFIG_FILE WORK_DIR SIMULATION_DIR)
  set(multi_value_args ARGS DEPENDS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_resolve_path_default(SIMULATION_DIR obj_dir WORK_DIR)
  _fi_resolve_path_default(FAULT_CAMPAIGN_OUT fault_campaign_out.csv SIMULATION_DIR)

  if(NOT FI_OUTPUT)
    set(FI_OUTPUT "${FI_FAULT_CAMPAIGN_OUT}")
  endif()

  if(FI_CONFIG_FILE)
    set(FI_CONFIG "${FI_CONFIG_FILE}")
  elseif(NOT FI_CONFIG AND NOT FI_ARGS)
    set(FI_CONFIG "${FI_SIMULATION_DIR}/config.json")
  endif()

  if(NOT FI_WORK_DIR)
    _fi_default_arg(FI_WORK_DIR WORK_DIR)
  endif()

  set(_command "$<TARGET_FILE:FaultGenerationTool>" ${FI_ARGS})
  if(FI_CONFIG)
    list(APPEND _command "--config_file" "${FI_CONFIG}")
  endif()

  get_filename_component(_output_dir "${FI_OUTPUT}" DIRECTORY)
  get_filename_component(_fault_campaign_out_dir "${FI_FAULT_CAMPAIGN_OUT}" DIRECTORY)
  set(_make_dirs "${FI_WORK_DIR}")
  if(NOT "${_output_dir}" STREQUAL "")
    list(APPEND _make_dirs "${_output_dir}")
  endif()
  if(NOT "${_fault_campaign_out_dir}" STREQUAL "")
    list(APPEND _make_dirs "${_fault_campaign_out_dir}")
  endif()

  add_custom_command(
    OUTPUT "${FI_OUTPUT}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${FI_OUTPUT}" "${FI_FAULT_CAMPAIGN_OUT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory ${_make_dirs}
    COMMAND ${_command}
    COMMAND "${CMAKE_COMMAND}" -E touch "${FI_OUTPUT}"
    DEPENDS FaultGenerationTool ${FI_CONFIG} ${FI_DEPENDS}
    VERBATIM
  )
  add_custom_target("${NAME}" DEPENDS "${FI_OUTPUT}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
endfunction()

# fi_add_verilated_sim(<target>
#   [FAULT_INJECTION] [CAMPAIGN_FILE <path>]
#   [WORK_DIR <dir>] [SIMULATION_DIR <dir>]
#   [SOURCES <files...>] [DEPENDS <deps...>])
#
# Builds a Verilator binary in ${SIMULATION_DIR}.
#
# Required, explicit or implicit:
#   WORK_DIR          Command working directory.
#   SIMULATION_DIR   Verilator --Mdir and output directory. Defaults to
#                    ${WORK_DIR}/obj_dir.
#   SOURCES           Verilator input sources. No implicit default is currently
#                    applied by this helper; tests usually pass WORKER_SOURCES.
#
# Fixed Verilator flags:
#   --binary --vpi --Mdir <SIMULATION_DIR>
#   -CFLAGS -g
#   -j <FI_E2E_JOBS>
#
# Fault-injection mode:
#   FAULT_INJECTION adds FaultInjector.sv, links libFaultInjector, defines
#   FAULT_INJECTION_ENABLE, and sets FAULT_INJECTION_CAMPAIGN_FILE.
#   CAMPAIGN_FILE may be explicit; otherwise it defaults to FAULT_CAMPAIGN_OUT,
#   which defaults to ${SIMULATION_DIR}/fault_campaign_out.csv.
#
# Trace mode:
#   TRACE adds --trace --trace-vcd to verilator call
#
# Output:
#   ${SIMULATION_DIR}/Vtop is exposed as <target>_EXECUTABLE in parent scope.
#   A stamp file records successful Verilator completion.
function(fi_add_verilated_sim NAME)
  set(options FAULT_INJECTION TRACE NO_MAIN)
  set(one_value_args CAMPAIGN_FILE WORK_DIR SIMULATION_DIR)
  set(multi_value_args SOURCES DEPENDS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_resolve_path_default(SIMULATION_DIR obj_dir WORK_DIR)
  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  if(NOT FI_SOURCES)
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} requires SOURCES")
  endif()

  set(_exe "${FI_SIMULATION_DIR}/Vtop")
  set(_mdir "${FI_SIMULATION_DIR}")
  set(_args)
  set(_depends ${FI_DEPENDS} ${FI_SOURCES})
  if(FI_FAULT_INJECTION)
    if(NOT FI_CAMPAIGN_FILE)
      _fi_resolve_path_default(FAULT_CAMPAIGN_OUT fault_campaign_out.csv SIMULATION_DIR)
      set(FI_CAMPAIGN_FILE "${FI_FAULT_CAMPAIGN_OUT}")
    endif()
    list(APPEND _args "${FI_E2E_FAULT_INJECTOR_DIR}/FaultInjector.sv")
    list(APPEND _args -LDFLAGS "-L$<TARGET_FILE_DIR:FaultInjector> -lFaultInjector")
    list(APPEND _args -DFAULT_INJECTION_ENABLE)
    list(APPEND _args "-DFAULT_INJECTION_CAMPAIGN_FILE=\"${FI_CAMPAIGN_FILE}\"")
    list(APPEND _depends FaultInjector "${FI_E2E_FAULT_INJECTOR_DIR}/FaultInjector.sv")
  endif()
  if(FI_TRACE)
    list(APPEND _args --trace --trace-vcd)
  endif()

  if(FI_NO_MAIN)
    set(_build_arg --build --exe --cc --timing)
  else()
    set(_build_arg "--binary")
  endif()

  add_custom_command(
    OUTPUT "${_exe}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${FI_SIMULATION_DIR}" "${FI_WORK_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${FI_SIMULATION_DIR}/CMakeFiles" "${FI_SIMULATION_DIR}/Vtop.dir"
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${FI_SIMULATION_DIR}/Vtop" "${FI_SIMULATION_DIR}/Vtop__ALL.a" "${FI_SIMULATION_DIR}/Vtop.mk" "${FI_SIMULATION_DIR}/Vtop_classes.mk"
    COMMAND
      "${VERILATOR_EXECUTABLE}"
      ${_build_arg} --vpi --Mdir "${_mdir}"
      -CFLAGS "-g"
      -j "${FI_E2E_JOBS}"
      ${FI_SOURCES}
      ${_args}
    DEPENDS ${_depends}
    VERBATIM
  )

  add_custom_target("${NAME}" DEPENDS "${_exe}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
  set("${NAME}_EXECUTABLE" "${_exe}" PARENT_SCOPE)
endfunction()

# fi_add_sim_run(<target>
#   [EXPECT_MISMATCH] [EXECUTABLE <path>] [LOG <path>]
#   [WORK_DIR <dir>] [SIMULATION_DIR <dir>]
#   [ARGS <args...>] [DEPENDS <deps...>])
#
# Runs a simulation executable through run_and_check_log.cmake.
#
# Required, explicit or implicit:
#   WORK_DIR          Command working directory.
#   SIMULATION_DIR   Used for executable/log defaults. Defaults to
#                    ${WORK_DIR}/obj_dir.
#
# Optional/implicit:
#   EXECUTABLE        Defaults to ${SIMULATION_DIR}/Vtop.
#   LOG               Defaults to ${SIMULATION_DIR}/run.log.
#   ARGS              Extra runtime arguments appended to EXECUTABLE.
#   EXPECT_MISMATCH   Require the run output to contain "Mismatch"; without it,
#                    "Mismatch" is treated as failure.
#   EXPECT_VCD_GOLDENFILE <file_path>
#                     Require the vlt_dump.vcd run output to be exactly the same
#                     as <file_path>.
#
# The log is written by the script, but the CMake output is a .stamp file. The
# stamp is touched only after the script succeeds, so failed runs cannot leave a
# stale successful output just because run.log was written.
function(fi_add_sim_run NAME)
  set(options EXPECT_MISMATCH)
  set(one_value_args EXECUTABLE LOG WORK_DIR SIMULATION_DIR EXPECT_VCD_GOLDENFILE)
  set(multi_value_args ARGS DEPENDS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  _fi_resolve_path_default(SIMULATION_DIR obj_dir WORK_DIR)
  _fi_resolve_path_default(EXECUTABLE Vtop SIMULATION_DIR)
  _fi_resolve_path_default(LOG run.log SIMULATION_DIR)

  set(_stamp "${FI_LOG}.stamp")
  set(_args)
  if(FI_EXPECT_MISMATCH)
    list(APPEND _args "-DFI_EXPECT_MISMATCH=ON")
  endif()
  if(FI_EXPECT_VCD_GOLDENFILE)
    set(_expect_vcd_arg "${FI_WORK_DIR}/logs/vlt_dump.vcd\\;${FI_EXPECT_VCD_GOLDENFILE}")
    list(APPEND _args "-DFI_EXPECT_VCD_GOLDENFILE=${_expect_vcd_arg}")
  endif()

  add_custom_command(
    OUTPUT "${_stamp}"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DFI_COMMAND=${FI_EXECUTABLE};${FI_ARGS}"
      "-DFI_WORK_DIR=${FI_WORK_DIR}"
      "-DFI_LOG=${FI_LOG}"
      ${_args}
      -P "${FI_E2E_SCRIPT_DIR}/run_and_check_log.cmake"
    COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
    DEPENDS ${FI_DEPENDS} "${FI_EXECUTABLE}"
    VERBATIM
  )
  add_custom_target("${NAME}" DEPENDS "${_stamp}")
endfunction()

# fi_add_multi_campaign_runs(<target>
#   [CAMPAIGN_DIR <dir>] [LOG_DIR <dir>]
#   [WORK_DIR <dir>] [SIMULATION_DIR <dir>]
#   [VERILATOR_SOURCES <files...>] [DEPENDS <deps...>])
#
# Runs every campaign file in a directory through run_multi_campaigns.cmake.
# This is currently used by the worker-multiple scenario.
#
# Required, explicit or implicit:
#   WORK_DIR            Command working directory.
#   SIMULATION_DIR     Per-run output root. Defaults to ${WORK_DIR}/obj_dir.
#   VERILATOR_SOURCES  Defaults from WORKER_SOURCES.
#
# Optional/implicit:
#   CAMPAIGN_DIR       Defaults to existing CAMPAIGN_DIR, or
#                      ${SIMULATION_DIR}/fault_campaign.
#   LOG_DIR            Defaults to existing LOG_DIR, or
#                      ${SIMULATION_DIR}/logs.
#
# The build-time script creates one Verilator build directory and run log per
# campaign, then fails with a list of campaigns that did not report "Mismatch".
function(fi_add_multi_campaign_runs NAME)
  set(options)
  set(one_value_args CAMPAIGN_DIR LOG_DIR WORK_DIR SIMULATION_DIR)
  set(multi_value_args VERILATOR_SOURCES DEPENDS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_resolve_path_default(SIMULATION_DIR obj_dir WORK_DIR)
  _fi_resolve_path_default(CAMPAIGN_DIR fault_campaign SIMULATION_DIR)
  _fi_resolve_path_default(LOG_DIR logs SIMULATION_DIR)
  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  _fi_default_arg(FI_VERILATOR_SOURCES WORKER_SOURCES)

  set(_stamp "${FI_LOG_DIR}/${NAME}.stamp")
  add_custom_command(
    OUTPUT "${_stamp}"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DVERILATOR_EXECUTABLE=${VERILATOR_EXECUTABLE}"
      "-DFI_JOBS=${FI_E2E_JOBS}"
      "-DFI_CAMPAIGN_DIR=${FI_CAMPAIGN_DIR}"
      "-DFI_LOG_DIR=${FI_LOG_DIR}"
      "-DFI_WORK_DIR=${FI_WORK_DIR}"
      "-DFI_FAULT_INJECTOR_SV=${FI_E2E_FAULT_INJECTOR_DIR}/FaultInjector.sv"
      "-DFI_FAULT_INJECTOR_LIB_DIR=$<TARGET_FILE_DIR:FaultInjector>"
      "-DFI_VERILATOR_SOURCES=$<JOIN:${FI_VERILATOR_SOURCES},;>"
      -P "${FI_E2E_SCRIPT_DIR}/run_multi_campaigns.cmake"
    COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
    DEPENDS FaultInjector ${FI_DEPENDS} ${FI_VERILATOR_SOURCES}
    VERBATIM
  )
  add_custom_target("${NAME}" DEPENDS "${_stamp}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
endfunction()
