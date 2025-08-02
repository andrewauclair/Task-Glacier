project("task-glacier-server")

FetchContent_Declare(
    curlpp
    GIT_SHALLOW     TRUE
    GIT_REPOSITORY  https://github.com/jpbarrette/curlpp.git
    GIT_TAG         d080e301ec71061c46049d23fedfb8106be3153f
)

set(CURLPP_BUILD_SHARED_LIBS ON)

FetchContent_MakeAvailable(curlpp)

add_executable(task-glacier-server main.cpp packet_sender_impl.hpp)

set_target_properties(task-glacier-server PROPERTIES CXX_STANDARD 23)

target_link_libraries(task-glacier-server PRIVATE
 	task-glacier-server-lib
 	sockpp-static
    curlpp::curlpp
)
