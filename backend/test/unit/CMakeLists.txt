project("task-glacier-unit-tests")

add_executable(task-glacier-unit-tests 
	task_test.cpp
	packet_test.cpp
	api_test.cpp
)

set_target_properties(task-glacier-unit-tests PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-unit-tests PRIVATE
 	task-glacier-server-lib
  	task-glacier-cli-lib
	libassert::assert
	Catch2::Catch2WithMain
)

#list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

#include(CTest)
#include(Catch)
#catch_discover_tests(task-glacier-unit-tests)
