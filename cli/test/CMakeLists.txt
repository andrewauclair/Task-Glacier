cmake_minimum_required (VERSION 3.27)

add_executable(micro_task_cli_test  "task_test.cpp")

set_target_properties(micro_task_cli_test PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_cli_test PRIVATE
	Catch2::Catch2WithMain micro_task_lib
)
