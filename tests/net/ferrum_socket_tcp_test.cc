#include <gtest/gtest.h>
#include "../../../../src/net/ferrum_socket_tcp.h"

#define loop(var, a, x)                           \
    var = a;                                      \
    while (var-- && (x))                          \
    {                                             \
        usleep(100);                              \
        uv_run(uv_default_loop(), UV_RUN_NOWAIT); \
    }

const int32_t TCPSERVER_PORT = 9999;

using namespace ferrum::io::net;

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
