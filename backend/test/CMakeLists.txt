﻿project("task-glacier-tests")

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
