cmake_minimum_required (VERSION 3.27)

project("micro-task-cli-test")

add_executable(micro_task_cli_test 
	task_test.cpp
	packet_test.cpp
	api_test.cpp
)

set_target_properties(micro_task_cli_test PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_cli_test PRIVATE
	micro_task_lib
	libassert::assert
	Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)
catch_discover_tests(micro_task_cli_test)