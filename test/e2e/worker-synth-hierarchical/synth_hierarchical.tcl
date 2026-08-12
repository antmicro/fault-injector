# SPDX-License-Identifier: Apache-2.0

proc env_var_exists_and_non_empty { env_var } {
  return [expr { [info exists ::env($env_var)] && ![string equal $::env($env_var) ""] }]
}

yosys -import

if { [env_var_exists_and_non_empty LIB_FILES] } {
read_liberty -overwrite -setattr liberty_cell -lib {*}$::env(LIB_FILES)
}

read_verilog_file_list -F $::env(DESIGN_FILE_LIST)

yosys hierarchy -check -auto-top
yosys proc
yosys check
yosys flatten -noscopeinfo
yosys synth
yosys opt

yosys rename -wire

if { [env_var_exists_and_non_empty LIB_FILES] } {
  set lib_args ""
  foreach lib $::env(LIB_FILES) {
    append lib_args "-liberty $lib "
  }

  dfflibmap {*}$lib_args

  # Consider using custom ABC script
  abc {*}$lib_args
}

# Make sure we mapped all the cells to the target library
check -assert -mapped
write_json $::env(JSON_NETLIST)
exit
