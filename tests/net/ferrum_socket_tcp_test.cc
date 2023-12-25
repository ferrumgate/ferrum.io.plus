#include "../../../../src/net/ferrum_socket_tcp.h"

#include <gtest/gtest.h>

#include "../../../../tests/integration/server_client/tcpecho.h"

#define loop(var, a, x)                       \
  var = a;                                    \
  while(var-- && (x)) {                       \
    usleep(100);                              \
    uv_run(uv_default_loop(), UV_RUN_NOWAIT); \
  }

const int32_t TCPSERVER_PORT = 9999;

using namespace ferrum::io::net;
using namespace ferrum::io::common;
using namespace ferrum::io::memory;
using namespace ferrum::io::error;

class FerrumSocketTcpTest : public testing::Test {
 protected:
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    uv_loop_close(uv_default_loop());
    FuncTable::reset();
    tcp_echo_stop();
    tcp_echo_close_client();
    tcp_echo_close_server();
  }
};

TEST_F(FerrumSocketTcpTest, constructor_client) {
  int32_t counter;
  {
    auto addr = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
    loop(counter, 100, true);  // wait for loop cycle
  }
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, constructor_throws_exception) {
  int32_t counter;
  FuncTable::uv_tcp_init = [](uv_loop_t *, uv_tcp_t *) -> int { return -2; };

  EXPECT_ANY_THROW(FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080}));
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, open_throws_exception_uv_tcp_connection) {
  int32_t counter;
  FuncTable::uv_tcp_connect = [](uv_connect_t *, uv_tcp_t *,
                                 const struct sockaddr *,
                                 uv_connect_cb) -> int { return -2; };

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080});
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_ANY_THROW(socket.open());
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, open_throws_exception_uv_fileno) {
  int32_t counter;
  FuncTable::uv_fileno = [](const uv_handle_t *, uv_os_fd_t *) -> int {
    return -2;
  };

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080}, true);
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_ANY_THROW(socket.open());
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, open_throws_exception_uv_bind) {
  int32_t counter;
  FuncTable::uv_tcp_bind = [](uv_tcp_t *, const struct sockaddr *,
                              unsigned int) -> int { return -2; };

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 8080}, true);
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_ANY_THROW(socket.open());
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, socket_on_connect_called) {
  auto result = tcp_echo_start(9998, 1);
  tcp_echo_listen();
  int counter = 0;

  struct CustomShared : public FerrumShared {
    bool connected{false};
  };

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9998});
  auto context = std::make_shared<CustomShared>(CustomShared{});
  socket.share(context);
  socket.on_open([](Shared &shared) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->connected = true;
  });
  loop(counter, 100, true);  // wait for loop cycle
  socket.open();
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_TRUE(context->connected == true);
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, socket_on_connect_failed) {
  auto result = tcp_echo_start(9998, 1);
  tcp_echo_listen();
  int counter = 0;

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
  };

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9997});
  auto context = std::make_shared<CustomShared>(CustomShared{});
  socket.share(context);
  socket.on_open([](Shared &shared) noexcept {
    auto cls = reinterpret_cast<CustomShared *>(shared.get());
    cls->connected = true;
  });
  socket.on_error([](Shared &shared, auto error) noexcept {
    auto cls = reinterpret_cast<CustomShared *>(shared.get());
    cls->onError = true;
  });

  loop(counter, 100, true);  // wait for loop cycle
  socket.open();
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_TRUE(context->connected == false);
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, socket_on_connect_uv_read_start_failed) {
  auto result = tcp_echo_start(9998, 1);
  tcp_echo_listen();
  int counter = 0;

  FuncTable::uv_read_start = [](uv_stream_t *, uv_alloc_cb, uv_read_cb) -> int {
    return -2;
  };

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
  };

  auto socket = FerrumSocketTcp(FerrumAddr{"127.0.0.1", 9998});
  auto context = std::make_shared<CustomShared>(CustomShared{});
  socket.share(context);
  socket.on_open([](Shared &shared) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->connected = true;
  });
  socket.on_error([](Shared &shared, auto error) noexcept {
    auto cls = static_cast<CustomShared *>(shared.get());
    cls->onError = true;
  });
  loop(counter, 100, true);  // wait for loop cycle
  socket.open();
  loop(counter, 100, true);  // wait for loop cycle
  EXPECT_TRUE(context->connected == false);
  EXPECT_TRUE(context->onError == true);
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketTcpTest, integration_with_nginx) {
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
    socket.open();
    loop(counter, 1000,
         !context->connected);  // wait for loop cycle
                                // ASSERT_TRUE(context->connected);

    socket.write(BufferByte(reinterpret_cast<const std::byte *>(head),
                            strlen(head) + 1));
    loop(counter, 100, true);  // wait for loop cycle
    ASSERT_TRUE(!context->data_s.empty());
  }
}

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
    socket.open();
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
  socket.open();
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
  socket.open();
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

  server->open();
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