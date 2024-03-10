cmake_minimum_required (VERSION 3.27)

project("micro-task-cli-test")

add_executable(micro_task_cli_test "task_test.cpp")

set_target_properties(micro_task_cli_test PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_cli_test PRIVATE
	Catch2::Catch2WithMain micro_task_lib
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(micro_task_cli_test PRIVATE "-stdlib=libc++")
	target_link_options(micro_task_cli_test PRIVATE "-stdlib=libc++")
endif()

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)
catch_discover_tests(micro_task_cli_test)