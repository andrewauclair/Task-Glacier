add_library (micro_task_server_lib STATIC 
	api.cpp
	server.cpp 
	api.hpp
	server.hpp
)

target_include_directories(micro_task_server_lib PUBLIC .)

set_target_properties(micro_task_server_lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_server_lib PUBLIC
	strong_type micro_task_common
)
