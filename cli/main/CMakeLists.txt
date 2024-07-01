project("micro-task-cli")

add_executable(micro_task_cli main.cpp)

set_target_properties(micro_task_cli PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_cli PRIVATE
	micro_task_lib sockpp-static
)
