cmake_minimum_required (VERSION 3.27)

add_library (micro_task_lib STATIC "src/lib.cpp")

target_include_directories(micro_task_lib PUBLIC include)

set_target_properties(micro_task_lib PROPERTIES CXX_STANDARD 23)
