# SPDX-License-Identifier: Apache-2.0

add_executable(TestEventParsing
    TestEventParsing.cpp
    vpi_user_mock.cpp
)
target_include_directories(TestEventParsing PRIVATE
    ../../src
)
target_link_libraries(TestEventParsing PRIVATE GTest::gtest_main faultergeist-inject)
add_test(NAME unit.eventParsing COMMAND TestEventParsing)
