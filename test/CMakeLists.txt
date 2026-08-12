# SPDX-License-Identifier: Apache-2.0

set(PDK_DEPS "")
foreach(lib_archive lib_file IN ZIP_LISTS FI_E2E_ASAP7_LIBERTY_ARCHIVES FI_E2E_LIB_FILES)
 get_filename_component(_fi_e2e_lib_file_dir ${lib_file} DIRECTORY)
 get_filename_component(_fi_e2e_lib_file_name ${lib_file} NAME_WE)
 if(ASAP7_LIBERTY_CACHE_PATH)
   add_custom_command(
     OUTPUT ${lib_file}
     COMMAND "${CMAKE_COMMAND}" -E make_directory "${_fi_e2e_lib_file_dir}"
     COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${ASAP7_LIBERTY_CACHE_PATH}" ${lib_file}
     DEPENDS "${ASAP7_LIBERTY_CACHE_PATH}"
     VERBATIM
   )
 else()
   add_custom_command(
     OUTPUT ${lib_file}
     COMMAND "${CMAKE_COMMAND}" -E make_directory "${_fi_e2e_lib_file_dir}"
     COMMAND "${SEVENZIP_EXECUTABLE}" x -y "-o${_fi_e2e_lib_file_dir}" ${lib_archive}
     DEPENDS ${lib_archive}
     VERBATIM
   )
 endif()
add_custom_target("${_fi_e2e_lib_file_name}_target" DEPENDS ${lib_file})
list(APPEND PDK_DEPS "${_fi_e2e_lib_file_name}_target")
endforeach()

add_custom_target(${FI_E2E_PDK_TARGET} DEPENDS "${PDK_DEPS}")
set(LIB_FILES "${FI_E2E_LIB_FILES}")

# Worker
add_subdirectory(e2e/worker-basic)
add_subdirectory(e2e/worker-multiple)
add_subdirectory(e2e/worker-synth-hierarchical)
add_subdirectory(e2e/worker-synth-regression)

# VeeR
add_subdirectory(e2e/veer-basic)
