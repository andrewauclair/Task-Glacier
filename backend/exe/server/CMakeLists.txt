project("micro-task-server")

add_executable(micro_task_server main.cpp)

set_target_properties(micro_task_server PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_server PRIVATE
	micro_task_common micro_task_server_lib sockpp-static
)
