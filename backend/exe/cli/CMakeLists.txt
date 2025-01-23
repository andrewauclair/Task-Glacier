project("task-glacier-cli")

add_executable(task-glacier-cli main.cpp)

set_target_properties(task-glacier-cli PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-cli PRIVATE
	task-glacier-cli-lib sockpp-static
)
