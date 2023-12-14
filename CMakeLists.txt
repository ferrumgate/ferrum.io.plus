cmake_minimum_required(VERSION 3.0.0)
project(ferrum.io.plus VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
    libuv
    URL https://github.com/libuv/libuv/archive/refs/tags/v1.47.0.zip
)

FetchContent_MakeAvailable(libuv)
include_directories(${CMAKE_BINARY_DIR}/_deps/libuv-src/include)

include(CTest)

add_subdirectory(src)
add_subdirectory(tests)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
