#include "../../../../src/net/ferrum_socket_udp.h"

#include <gtest/gtest.h>

#include "../../../../tests/integration/server_client/udpecho.h"

#define loop(var, a, x)                       \
  var = a;                                    \
  while(var-- && (x)) {                       \
    usleep(100);                              \
    uv_run(uv_default_loop(), UV_RUN_NOWAIT); \
  }

const int32_t UDPSERVER_PORT = 9988;

using namespace ferrum::io::net;
using namespace ferrum::io::common;
using namespace ferrum::io::memory;
using namespace ferrum::io::error;

class FerrumSocketUdpTest : public testing::Test {
 protected:
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    uv_loop_close(uv_default_loop());
    FuncTable::reset();
    udp_echo_close();
  }
};

TEST_F(FerrumSocketUdpTest, constructor_client) {
  int32_t counter;
  {
    auto addr = FerrumSocketUdp(FerrumAddr{"127.0.0.1", 8080});
    loop(counter, 100, true);  // wait for loop cycle
  }
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketUdpTest, constructor_throws_exception) {
  int32_t counter;
  FuncTable::uv_udp_init = [](uv_loop_t *, uv_udp_t *) -> int { return -2; };

  EXPECT_ANY_THROW(FerrumSocketUdp(FerrumAddr{"127.0.0.1", 8080}));
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketUdpTest, open_throws_exception_uv_bind) {
  int32_t counter;
  FuncTable::uv_udp_bind = [](uv_udp_t *, const struct sockaddr *,
                              unsigned int) -> int { return -2; };

  auto socket = FerrumSocketUdp(FerrumAddr{"127.0.0.1", 8080}, true);
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_ANY_THROW(socket.open({}));
  loop(counter, 100, true);  // wait for loop cycle
}
TEST_F(FerrumSocketUdpTest, open_throws_exception_uv_read_start) {
  int32_t counter;
  FuncTable::uv_udp_read_start = [](uv_udp_t *, uv_alloc_cb,
                                    uv_udp_recv_cb) -> int { return -2; };

  auto socket = FerrumSocketUdp(FerrumAddr{"127.0.0.1", 8080}, true);
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_ANY_THROW(socket.open({}));
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketUdpTest, uv_write_failed) {
  auto result = udp_echo_start(9998);

  int32_t counter = 0;

  FuncTable::uv_write = [](uv_write_t *, uv_stream_t *, const uv_buf_t[],
                           unsigned int, uv_write_cb) -> int { return -2; };

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
    std::vector<std::byte> data;
    std::string data_s;
    int counter{0};
  };
  const char *msg = "hello";
  auto socket = FerrumSocketUdp(FerrumAddr{"127.0.0.1", 9998});
  auto context = std::make_shared<CustomShared>(CustomShared{});
  socket.share(context);
  socket.on_error([](Shared &shared, BaseException ex) noexcept {
    std::cout << ex.get_message() << std::endl;
  });
  socket.on_read([](Shared &shared, const BufferByte &data) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    auto str = data.to_string();
    cls->data_s.append(str);
  });
  socket.on_open([](Shared &shared) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->connected = true;
    cls->counter++;
  });

  loop(counter, 100, true);  // wait for loop cycle
  socket.open({});
  loop(counter, 1000,
       !context->connected);  // wait for loop cycle
                              // ASSERT_TRUE(context->connected);

  ASSERT_ANY_THROW(socket.write(
      BufferByte(reinterpret_cast<const std::byte *>(msg), strlen(msg) + 1)));
  loop(counter, 100, true);  // wait for loop cycle
  ASSERT_TRUE(context->data_s.empty());
}

TEST_F(FerrumSocketUdpTest, start_as_server) {
  int32_t counter = 0;

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
    std::vector<std::byte> data;
    std::string data_s;
    int counter{0};
    FerrumSocketUdp &ref;
  };
  // html header get message
  const char *msg = "ping";
  auto addr = FerrumAddr{"127.0.0.1", 9595};
  auto socket = FerrumSocketUdp(FerrumAddr{addr});
  auto context = std::make_shared<CustomShared>(CustomShared{.ref = socket});
  socket.share(context);
  socket.on_error([](Shared &shared, BaseException ex) noexcept {
    std::cout << ex.get_message() << std::endl;
    });
  socket.on_read([](Shared &shared, const BufferByte &data) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    auto str = data.to_string();
    cls->data_s.append(str);
    const char *msg = "pong";
    cls->ref.write(
        BufferByte(reinterpret_cast<const std::byte *>(msg), strlen(msg) + 1));
  });
  socket.on_open([](Shared &shared) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->connected = true;
    cls->counter++;
  });

  loop(counter, 100, true);  // wait for loop cycle
  socket.open({});
  loop(counter, 1000,
       true);  // wait for loop cycle

  udp_echo_send2("msg", addr.get_addr4());
  loop(counter, 100, true);  // wait for loop cycle
  ASSERT_TRUE(!context->data_s.empty());
  char response[65536];
  udp_echo_recv(response);
  loop(counter, 100, true);  // wait for loop cycle
  ASSERT_EQ(response, "pong");
  loop(counter, 100, true);  // wait for loop cycle
}

/*
TEST_F(FerrumSocketTcpTest, integration_with_nginx_download) {
  int32_t counter = 0;

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
    std::vector<std::byte> data;
    std::string data_s;
    int counter{0};
  };
  // html header get message
  const char *head =
      "GET /100M.ignore.txt HTTP/1.0\r\n\
Host: nodejs.org\r\n\
User-Agent: ferrum\r\n\
Accept: text/html\r\n\
\r\n";

  for(size_t i = 0; i < 10; i++) {
    auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
    auto context = std::make_shared<CustomShared>(CustomShared{});
    socket.share(context);
    socket.on_error([](Shared &shared, BaseException ex) noexcept {
      std::cout << ex.get_message() << std::endl;
    });
    socket.on_read([](Shared &shared, const BufferByte &data) noexcept {
      auto cls = static_cast<CustomShared *>(shared.get());
      auto str = data.to_string();
      cls->data_s.append(str);
    });
    socket.on_open([](Shared &shared) noexcept {
      auto cls = static_cast<CustomShared *>(shared.get());
      cls->connected = true;
      cls->counter++;
    });

    loop(counter, 100, true);  // wait for loop cycle
    socket.open({});
    loop(counter, 1000,
         !context->connected);  // wait for loop cycle
                                // ASSERT_TRUE(context->connected);

    socket.write(BufferByte(reinterpret_cast<const std::byte *>(head),
                            strlen(head) + 1));
    loop(counter, 1000, true);  // wait for loop cycle
    ASSERT_TRUE(!context->data_s.empty());
  }
}

TEST_F(FerrumSocketTcpTest, bind) {
  int32_t counter = 0;

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
    std::vector<std::byte> data;
    std::string data_s;
    int counter{0};
  };
  // html header get message
  const char *head =
      "GET / HTTP/1.0\r\n\
Host: nodejs.org\r\n\
User-Agent: ferrum\r\n\
Accept: text/html\r\n\
\r\n";

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
  auto context = std::make_shared<CustomShared>(CustomShared{});
  socket.share(context);
  socket.bind(FerrumAddr{"127.0.0.1", 25000});
  socket.on_error([](Shared &shared, BaseException ex) noexcept {
    std::cout << ex.get_message() << std::endl;
  });
  socket.on_read([](Shared &shared, const BufferByte &data) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    auto str = data.to_string();
    cls->data_s.append(str);
  });
  socket.on_open([](Shared &shared) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->connected = true;
    cls->counter++;
  });

  loop(counter, 100, true);  // wait for loop cycle
  socket.open({});
  loop(counter, 1000, !context->connected);  // wait for loop cycle
                                             // ASSERT_TRUE(context->connected);
  socket.write(
      BufferByte(reinterpret_cast<const std::byte *>(head), strlen(head) + 1));
  loop(counter, 100, true);  // wait for loop cycle
  ASSERT_TRUE(!context->data_s.empty());
  socket.close();
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, on_close) {
  auto result = tcp_echo_start(9992, 1);
  tcp_echo_listen();
  int counter = 0;

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
    FerrumSocketTcp &socket;
  };

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9992});
  auto context = std::make_shared<CustomShared>(CustomShared{.socket = socket});
  socket.share(context);
  socket.on_open([](Shared &shared) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->connected = true;
  });
  socket.on_error([](Shared &shared, auto error) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->onError = true;
    cls->socket.close();
  });

  loop(counter, 100, true);  // wait for loop cycle
  socket.open({});
  loop(counter, 1000, !context->connected);  // wait for loop cycle
  EXPECT_TRUE(context->connected);
  tcp_echo_stop();
  tcp_echo_close_client();
  tcp_echo_close_server();
  loop(counter, 1000, !context->onError);  // wait for loop cycle
  EXPECT_TRUE(context->onError);
  loop(counter, 100, true);  // wait for loop cycle
}

std::string exec(const char *cmd) {
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if(!pipe) return "ERROR";
  char buffer[128];
  std::string result = "";
  while(!feof(pipe.get())) {
    if(fgets(buffer, 128, pipe.get()) != NULL) result += buffer;
  }
  return result;
}

TEST_F(FerrumSocketTcpTest, server_mode) {
  int counter = 0;
  static int32_t lastCounter = 0;

  struct CustomContext;
  using CustomContextShared = std ::shared_ptr<CustomContext>;

  struct GlobalContext {
    std::list<CustomContextShared> sockets;
    bool isClientConnected{false};
    bool isClientReaded{false};
    bool isClientError{false};
    bool isClientClosed{false};
  };

  struct CustomContext : public FerrumShared {
    int32_t id{0};
    bool isConnected{false};
    bool isError{false};
    bool isClosed{false};
    bool isReaded{false};
    std::shared_ptr<FerrumSocketTcp> socket{nullptr};
    std::shared_ptr<GlobalContext> global{nullptr};
  };

  auto global = std::make_shared<GlobalContext>(GlobalContext{});
  auto server = std::make_shared<FerrumSocketTcp>(
      FerrumSocketTcp(FerrumAddr{"0.0.0.0", 9992}, true));
  auto contextServer = std::make_shared<CustomContext>(
      CustomContext{.socket = server, .global = global});

  server->share(contextServer);
  server->on_error([](Shared &shared, BaseException ex) noexcept {
    auto cls = static_cast<CustomContext *>(shared.get());
    cls->isError = true;
  });

  server->on_accept([](Shared &shared, FerrumSocketShared &client) noexcept {
    auto shared_ptr = std::reinterpret_pointer_cast<CustomContext>(shared);
    shared_ptr->isConnected = true;
    auto client_ptr = std::reinterpret_pointer_cast<FerrumSocketTcp>(client);
    auto contextClient =
        std::make_shared<CustomContext>(CustomContext{.id = lastCounter++,
                                                      .isConnected = true,
                                                      .isError = false,
                                                      .socket = client_ptr});
    contextClient->global = shared_ptr->global;
    contextClient->global->isClientConnected = true;
    shared_ptr->global->sockets.push_back(contextClient);

    client_ptr->share(contextClient);
    client_ptr->on_error([](Shared &shared, BaseException ex) noexcept {
      auto context = std::reinterpret_pointer_cast<CustomContext>(shared);
      context->global->isClientError = true;
      context->isError = true;
      context->socket->close();
    });
    client_ptr->on_read([](Shared &shared, const BufferByte &data) noexcept {
      auto context = std::reinterpret_pointer_cast<CustomContext>(shared);
      context->isReaded = true;
      context->global->isClientReaded = true;
      const char *msg = "hello";
      context->socket->write(BufferByte(
          reinterpret_cast<const std::byte *>(msg), strlen(msg) + 1));
    });

    client_ptr->on_close([](Shared &shared) noexcept {
      auto context = std::reinterpret_pointer_cast<CustomContext>(shared);
      context->global->isClientClosed = true;
      std::remove_if(
          context->global->sockets.begin(), context->global->sockets.end(),
          [&context](CustomContextShared &a) { return a->id == context->id; });
    });
  });

  server->open({});
  tcp_echo_start(9992, 0);
  tcp_echo_send("hello");

  loop(counter, 1000, !global->isClientReaded);  // wait for loop cycle
  server->close();
  tcp_echo_close_client();
  loop(counter, 1000, !global->isClientClosed);  // wait for loop cycle
  EXPECT_TRUE(global->isClientClosed);
  EXPECT_TRUE(global->isClientConnected);
  EXPECT_TRUE(global->isClientError);  // because socket closed
  EXPECT_TRUE(global->isClientReaded);
}

*/