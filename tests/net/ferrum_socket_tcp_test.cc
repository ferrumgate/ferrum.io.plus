#include <gtest/gtest.h>
#include "../../../../src/net/ferrum_socket_tcp.h"
#include "../../../../tests/integration/server_client/tcpecho.h"

#define loop(var, a, x)                           \
    var = a;                                      \
    while (var-- && (x))                          \
    {                                             \
        usleep(100);                              \
        uv_run(uv_default_loop(), UV_RUN_NOWAIT); \
    }

const int32_t TCPSERVER_PORT = 9999;

using namespace ferrum::io::net;
using namespace ferrum::io::common;

class FerrumSocketTcpTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override
    {
        uv_loop_close(uv_default_loop());
        FuncTable::reset();
    }
};

TEST(FerrumSocketTcp, constructor)
{
    int32_t counter;
    {
        auto addr = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
        loop(counter, 100, true); // wait for loop cycle
    }
    loop(counter, 100, true); // wait for loop cycle
}

TEST(FerrumSocketTcp, constructor_throws_exception)
{
    int32_t counter;
    FuncTable::uv_tcp_init = [](uv_loop_t *, uv_tcp_t *) -> int
    {
        return -2;
    };

    EXPECT_ANY_THROW(FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080}));
    loop(counter, 100, true); // wait for loop cycle
}
TEST(FerrumSocketTcp, constructor_throws_exception2)
{
    int32_t counter;
    FuncTable::uv_tcp_bind = [](uv_tcp_t *, const struct sockaddr *, unsigned int) -> int
    {
        return -2;
    };

    EXPECT_ANY_THROW(FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080}));
    loop(counter, 100, true); // wait for loop cycle
}

TEST(FerrumSocketTcp, open_throws_exception)
{

    int32_t counter;
    FuncTable::uv_tcp_connect = [](uv_connect_t *, uv_tcp_t *, const struct sockaddr *, uv_connect_cb) -> int
    {
        return -2;
    };

    auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
    loop(counter, 100, true); // wait for loop cycle
    EXPECT_ANY_THROW(socket.open());
    loop(counter, 100, true); // wait for loop cycle
}

TEST(FerrumSocketTcp, socket_on_connect_called)
{
    auto result = tcp_echo_start(9998, 1);
    tcp_echo_listen();

    int32_t counter;
    FuncTable::uv_tcp_connect = [](uv_connect_t *, uv_tcp_t *, const struct sockaddr *, uv_connect_cb) -> int
    {
        return -2;
    };

    auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9998});
    /* socket.on_open([&socket]() {

    }); */
    loop(counter, 100, true); // wait for loop cycle
    socket.open();
    loop(counter, 100, true); // wait for loop cycle
}
