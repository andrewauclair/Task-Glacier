cmake_minimum_required (VERSION 3.22)

set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project("task-glacier")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_VERSION "unknown")
endif()

if(DEFINED ENV{RELEASE_VERSION})
    set(APP_VERSION $ENV{RELEASE_VERSION})
else()
    set(APP_VERSION ${GIT_VERSION})
endif()

message(STATUS "Building version: ${APP_VERSION}")
add_compile_definitions(APP_VERSION="${APP_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH $ORIGIN)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE PATH "Where to place compiled static libraries.")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE PATH "Where to place compiled shared libraries.")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Where to place compiled executables.")

add_subdirectory(external)
add_subdirectory(lib)
add_subdirectory(exe)
add_subdirectory(test)
