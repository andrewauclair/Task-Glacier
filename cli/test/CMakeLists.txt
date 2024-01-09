cmake_minimum_required (VERSION 3.8)

add_executable(micro_task_cli_test  "task_test.cpp")

target_link_libraries(micro_task_cli_test PRIVATE
	Catch2::Catch2WithMain
)