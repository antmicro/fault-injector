# SPDX-License-Identifier: Apache-2.0

add_executable(TestEventParsing
    TestEventParsing.cpp
)
target_include_directories(TestEventParsing PRIVATE
    ../src
    ${VERILATOR_ROOT}/include/vltstd
)
target_link_libraries(TestEventParsing PRIVATE GTest::gtest_main FaultInjector)
add_test(NAME TestEventParsing COMMAND TestEventParsing)
