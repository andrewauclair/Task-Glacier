add_library (micro_task_server_lib STATIC 
	api.hpp		api.cpp
	server.cpp  server.hpp
	persist.hpp persist.cpp
)

target_include_directories(micro_task_server_lib PUBLIC .)

set_target_properties(micro_task_server_lib PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_server_lib PUBLIC
	strong_type micro_task_common
)
