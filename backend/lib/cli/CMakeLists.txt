add_library (task-glacier-cli STATIC 
	cli.cpp
)

target_include_directories(task-glacier-cli PUBLIC .)

set_target_properties(task-glacier-cli PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-cli PUBLIC
	strong_type micro_task_common sockpp-static
)
