
enable_testing()
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(
    hello_test
    hello_test.cc
    error/base_exception_test.cc
    log/logger_test.cc
    memory/buffer_test.cc
    net/ferrum_addr_test.cc
    net/ferrum_socket_tcp_test.cc
)

target_link_libraries(
    hello_test
    ferrum.io.plus
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hello_test)
