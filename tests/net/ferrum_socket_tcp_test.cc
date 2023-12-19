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
using namespace ferrum::io::memory;
using namespace ferrum::io::error;

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
        tcp_echo_stop();
    }
};

TEST_F(FerrumSocketTcpTest, constructor)
{
    int32_t counter;
    {
        auto addr = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
        loop(counter, 100, true); // wait for loop cycle
    }
    loop(counter, 100, true); // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, constructor_throws_exception)
{
    int32_t counter;
    FuncTable::uv_tcp_init = [](uv_loop_t *, uv_tcp_t *) -> int
    {
        return -2;
    };

    EXPECT_ANY_THROW(FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080}));
    loop(counter, 100, true); // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, open_throws_exception)
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

TEST_F(FerrumSocketTcpTest, socket_on_connect_called)
{
    auto result = tcp_echo_start(9998, 1);
    tcp_echo_listen();
    int counter = 0;

    struct CustomShared : public FerrumShared
    {
        bool connected{false};
    };

    auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9998});
    auto context = std::make_shared<CustomShared>(CustomShared{});
    socket.share(context);
    socket.on_open([](Shared &shared) noexcept
                   { auto cls = static_cast<CustomShared*>(shared.get());
                   cls->connected=true; });
    loop(counter, 100, true); // wait for loop cycle
    socket.open();
    loop(counter, 100, true); // wait for loop cycle
    EXPECT_TRUE(context->connected == true);
    loop(counter, 100, true); // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, socket_on_connect_failed)
{
    auto result = tcp_echo_start(9998, 1);
    tcp_echo_listen();
    int counter = 0;

    struct CustomShared : public FerrumShared
    {
        bool connected{false};
        bool onError{false};
    };

    auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9997});
    auto context = std::make_shared<CustomShared>(CustomShared{});
    socket.share(context);
    socket.on_open([](Shared &shared) noexcept
                   { 
                    auto cls = reinterpret_cast<CustomShared*>(shared.get());
                   cls->connected=true; });
    socket.on_error([](Shared &shared, auto error) noexcept
                    {

        auto cls=reinterpret_cast<CustomShared*>(shared.get());
        cls->onError=true; });

    loop(counter, 100, true); // wait for loop cycle
    socket.open();
    loop(counter, 100, true); // wait for loop cycle
    EXPECT_TRUE(context->connected == false);
    loop(counter, 100, true); // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, socket_on_connect_uv_read_start_failed)
{
    auto result = tcp_echo_start(9998, 1);
    tcp_echo_listen();
    int counter = 0;

    FuncTable::uv_read_start = [](uv_stream_t *,
                                  uv_alloc_cb,
                                  uv_read_cb) -> int
    {
        return -2;
    };

    struct CustomShared : public FerrumShared
    {
        bool connected{false};
        bool onError{false};
    };

    auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9998});
    auto context = std::make_shared<CustomShared>(CustomShared{});
    socket.share(context);
    socket.on_open([](Shared &shared) noexcept
                   { auto cls = static_cast<CustomShared*>(shared.get());
                   cls->connected=true; });
    socket.on_error([](Shared &shared, auto error) noexcept
                    {
        auto cls=static_cast<CustomShared*>(shared.get());
        cls->onError=true; });
    loop(counter, 100, true); // wait for loop cycle
    socket.open();
    loop(counter, 100, true); // wait for loop cycle
    EXPECT_TRUE(context->connected == false);
    EXPECT_TRUE(context->onError == true);
    loop(counter, 100, true); // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, integration_with_nginx)
{
    int32_t counter = 0;

    struct CustomShared : public FerrumShared
    {
        bool connected{false};
        bool onError{false};
        std::vector<std::byte> data;
        std::string data_s;
        int counter{0};
    };
    // html header get message
    const char *head = "GET / HTTP/1.0\r\n\
Host: nodejs.org\r\n\
User-Agent: ferrum\r\n\
Accept: text/html\r\n\
\r\n";

    for (size_t i = 0; i < 10; i++)
    {

        auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
        auto context = std::make_shared<CustomShared>(CustomShared{});
        socket.share(context);
        socket.on_error([](Shared &shared, BaseException ex) noexcept
                        { std::cout << ex.get_message() << std::endl; });
        socket.on_read([](Shared &shared, const BufferByte &data) noexcept
                       {
                        auto cls = static_cast<CustomShared *>(shared.get());
                        auto str=data.to_string();
                        cls->data_s.append(str); });
        socket.on_open([](Shared &shared) noexcept
                       { auto cls = static_cast<CustomShared*>(shared.get());
                    cls->connected=true;cls->counter++; });

        loop(counter, 100, true); // wait for loop cycle
        socket.open();
        loop(counter, 1000, !context->connected); // wait for loop cycle
                                                  // ASSERT_TRUE(context->connected);

        socket.write(BufferByte(reinterpret_cast<const std::byte *>(head), strlen(head) + 1));
        loop(counter, 100, true); // wait for loop cycle
        ASSERT_TRUE(!context->data_s.empty());
    }
}
