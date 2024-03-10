cmake_minimum_required (VERSION 3.27)

add_library (micro_task_lib STATIC "src/lib.cpp" "include/lib.h")

target_include_directories(micro_task_lib PUBLIC include)

set_target_properties(micro_task_lib PROPERTIES CXX_STANDARD 23)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(micro_task_lib PRIVATE "-stdlib=libc++")
	target_link_options(micro_task_lib PUBLIC "-stdlib=libc++")
endif()
