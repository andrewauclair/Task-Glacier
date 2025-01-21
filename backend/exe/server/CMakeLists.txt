project("micro-task-server")

add_executable(task-glacier main.cpp)

set_target_properties(task-glacier PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier PRIVATE
	micro_task_common micro_task_server_lib sockpp-static
)
