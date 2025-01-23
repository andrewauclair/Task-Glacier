add_library (task-glacier-cli-lib STATIC 
	cli.cpp
)

target_include_directories(task-glacier-cli-lib PUBLIC .)

set_target_properties(task-glacier-cli-lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-cli-lib PUBLIC
	task-glacier-common
	strong_type
 	sockpp-static
)
