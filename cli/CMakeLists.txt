cmake_minimum_required (VERSION 3.8)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project("micro-task")

add_subdirectory(external)
add_subdirectory(lib)
add_subdirectory(main)
add_subdirectory(test)