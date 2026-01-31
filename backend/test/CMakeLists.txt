project("task-glacier-tests")

add_executable(task-glacier-tests 
	api_test.cpp
	packet_test.cpp
	bugzilla_test.cpp
	sqlite_test.cpp

	utils.h
)

set_target_properties(task-glacier-tests PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-tests PRIVATE
 	task-glacier-server-lib
	cpptrace::cpptrace
	Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
message("------- module path -----------: " ${CMAKE_MODULE_PATH})

include(CTest)
include(Catch)
catch_discover_tests(task-glacier-tests)