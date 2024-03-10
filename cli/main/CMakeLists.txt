cmake_minimum_required(VERSION 3.27)

project("micro-task-cli")

add_executable(micro_task_cli "main.cpp" "packets.hpp")

set_target_properties(micro_task_cli PROPERTIES CXX_STANDARD 23)

target_link_libraries(micro_task_cli PRIVATE
	micro_task_lib sockpp-static
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(micro_task_cli PRIVATE "-stdlib=libc++")
	target_link_options(micro_task_cli PRIVATE "-stdlib=libc++")
endif()
