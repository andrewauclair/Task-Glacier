# CMakeList.txt : CMake project for micro-task-cli, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("micro-task-cli")

# Add source to this project's executable.
add_executable (micro_task_cli "main.cpp" "packets.hpp")

target_link_libraries(micro_task_cli PRIVATE micro_task_lib sockpp-static nlohmann_json)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET micro_task_cli PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
