project("task-glacier-server")

add_executable(task-glacier-server main.cpp)

set_target_properties(task-glacier-server PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-server PRIVATE
 	task-glacier-server-lib
 	sockpp-static
)
