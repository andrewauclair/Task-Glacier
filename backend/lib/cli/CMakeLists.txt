add_library (micro_task_cli_lib STATIC 
	cli.cpp
)

target_include_directories(micro_task_cli_lib PUBLIC .)

set_target_properties(micro_task_cli_lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_cli_lib PUBLIC
	strong_type micro_task_common
)
