# CMakeList.txt : CMake project for micro-task-cli, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.27)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("micro-task-cli")

# Add source to this project's executable.
add_library (micro_task_lib STATIC "src/lib.cpp")

target_include_directories(micro_task_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(micro_task_lib PROPERTIES CXX_STANDARD 23)

# TODO: Add tests and install targets if needed.
