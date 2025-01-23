cmake_minimum_required (VERSION 3.22)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project("task-glacier")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH $ORIGIN)
 
add_subdirectory(external)
add_subdirectory(lib/common)
add_subdirectory(lib/server)
add_subdirectory(lib/cli)
add_subdirectory(exe/server)
add_subdirectory(exe/cli)
add_subdirectory(test/unit)
add_subdirectory(test/integration)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE PATH "Where to place compiled static libraries.")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE PATH "Where to place compiled shared libraries.")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Where to place compiled executables.")
