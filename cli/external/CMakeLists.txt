Include(FetchContent)

cmake_policy(SET CMP0077 NEW)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.3.2
)

set(SOCKPP_BUILD_SHARED OFF)
set(SOCKPP_BUILD_STATIC ON)

FetchContent_Declare(
        sockpp
        GIT_REPOSITORY https://github.com/fpagliughi/sockpp.git
        GIT_TAG        v0.8.1
)

FetchContent_Declare(
        poco
        GIT_REPOSITORY https://github.com/pocoproject/poco
        GIT_TAG        poco-1.13.0-release
)

FetchContent_MakeAvailable(Catch2 sockpp poco)