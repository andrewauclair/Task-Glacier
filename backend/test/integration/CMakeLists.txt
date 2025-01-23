project("task-glacier-integration-tests")

add_executable(task-glacier-integration-tests 
	test.cpp
)

set_target_properties(task-glacier-integration-tests PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-integration-tests PRIVATE
	task-glacier-server-lib
	task-glacier-cli-lib
	libassert::assert
	Catch2::Catch2WithMain
)

#list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

#include(CTest)
#include(Catch)
#catch_discover_tests(task-glacier-integration-tests)
