project("task-glacier-tests")

add_executable(task-glacier-tests 
	api_test.cpp
	packet_test.cpp
)

set_target_properties(task-glacier-tests PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-tests PRIVATE
 	task-glacier-server-lib
	libassert::assert
	Catch2::Catch2WithMain
)
