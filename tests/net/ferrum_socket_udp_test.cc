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
    dup(STDERR_FILENO);
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

  {
    auto socket = FerrumSocketUdp(FerrumAddr{"127.0.0.1", 8080}, true);
    loop(counter, 100, true);  // wait for loop cycle
    EXPECT_ANY_THROW(socket.open({}));
  }
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketUdpTest, open_throws_exception_uv_read_start) {
  int32_t counter;
  FuncTable::uv_udp_read_start = [](uv_udp_t *, uv_alloc_cb,
                                    uv_udp_recv_cb) -> int { return -2; };

  {
    auto socket = FerrumSocketUdp(FerrumAddr{"127.0.0.1", 8080}, true);
    loop(counter, 100, true);  // wait for loop cycle
    EXPECT_ANY_THROW(socket.open({}));
  }
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketUdpTest, uv_write_failed) {
  auto result = udp_echo_start(9998);

  int32_t counter = 0;

  FuncTable::uv_udp_write = [](uv_udp_send_t *, uv_udp_t *, const uv_buf_t[],
                               unsigned int, const struct sockaddr *,
                               uv_udp_send_cb) -> int { return -2; };

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
    std::vector<std::byte> data;
    std::string data_s;
    int counter{0};
  };
  const char *msg = "hello";
  {
    auto socket = FerrumSocketUdp(FerrumAddr{"127.0.0.1", 9998});
    auto context = std::make_shared<CustomShared>(CustomShared{});
    socket.share(context);
    socket.on_error([](FerrumShared::Ptr &shared, BaseException ex) noexcept {
      std::cout << ex.get_message() << std::endl;
    });
    socket.on_read([](FerrumShared::Ptr &shared, const BufferByte &data,
                      FerrumAddr &&caddr) noexcept {
      auto cls = static_cast<CustomShared *>(shared.get());
      auto str = data.to_string();
      cls->data_s.append(str);
    });
    socket.on_open([](FerrumShared::Ptr &shared) noexcept {
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
        BufferByte(reinterpret_cast<const std::byte *>(msg), strlen(msg) + 1),
        socket.addr()));
    loop(counter, 100, true);  // wait for loop cycle
    ASSERT_TRUE(context->data_s.empty());
  }
  loop(counter, 100, true);  // wait for loop cycle
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

  auto result = udp_echo_start(9922);
  // html header get message
  const char *msg = "ping";
  {
    auto addr = FerrumAddr{"127.0.0.1", 9595};
    auto socket = FerrumSocketUdp(FerrumAddr{addr}, true);
    auto context = std::make_shared<CustomShared>(CustomShared{.ref = socket});
    socket.share(context);
    socket.on_error([](FerrumShared::Ptr &shared, BaseException ex) noexcept {
      std::cout << ex.get_message() << std::endl;
    });
    socket.on_read([](FerrumShared::Ptr &shared, const BufferByte &data,
                      FerrumAddr &&addr) noexcept {
      auto cls = static_cast<CustomShared *>(shared.get());
      auto str = data.to_string();
      cls->data_s.append(str);
      const char *msg = "pong";
      cls->ref.write(
          BufferByte(reinterpret_cast<const std::byte *>(msg), strlen(msg) + 1),
          addr);
    });
    socket.on_open([](FerrumShared::Ptr &shared) noexcept {
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
    ASSERT_STREQ(response, "pong");
    loop(counter, 100, true);  // wait for loop cycle
  }
  loop(counter, 100, true);  // wait for loop cycle
}

TEST_F(FerrumSocketUdpTest, start_as_client) {
  int32_t counter = 0;

  struct CustomShared : public FerrumShared {
    bool connected{false};
    bool onError{false};
    bool readed{false};
    std::vector<std::byte> data;
    std::string data_s;
    int counter{0};
    FerrumSocketUdp &ref;
  };

  auto result = udp_echo_start(9922);
  // html header get message

  {
    auto addr = FerrumAddr{"1.1.1.1", 53};
    auto socket = FerrumSocketUdp(FerrumAddr{addr}, false);
    auto context = std::make_shared<CustomShared>(CustomShared{.ref = socket});
    socket.share(context);
    socket.on_error([](FerrumShared::Ptr &shared, BaseException ex) noexcept {
      std::cout << ex.get_message() << std::endl;
    });
    socket.on_read([](FerrumShared::Ptr &shared, const BufferByte &data,
                      FerrumAddr &&addr) noexcept {
      auto cls = static_cast<CustomShared *>(shared.get());
      auto str = data.to_string();
      cls->readed = true;
    });
    socket.on_open([](FerrumShared::Ptr &shared) noexcept {
      auto cls = static_cast<CustomShared *>(shared.get());
      cls->connected = true;
      cls->counter++;
    });

    loop(counter, 100, true);  // wait for loop cycle
    socket.open({});
    loop(counter, 1000,
         true);  // wait for loop cycle
                 // www.google.com dns query
    unsigned char packet_bytes[] = {
        0x13, 0x0d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x03, 0x77, 0x77, 0x77, 0x06, 0x67, 0x6f, 0x6f, 0x67, 0x6c,
        0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
        0x00, 0x29, 0x04, 0xd0, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0c, 0x00,
        0x0a, 0x00, 0x08, 0x9f, 0x3b, 0x78, 0x4c, 0x94, 0x64, 0x5a, 0x28};
    socket.write(BufferByte(reinterpret_cast<const std::byte *>(packet_bytes),
                            sizeof(packet_bytes)),
                 addr);
    loop(counter, 100, true);  // wait for loop cycle

    ASSERT_TRUE(context->readed);
    loop(counter, 100, true);  // wait for loop cycle
  }
  loop(counter, 100, true);  // wait for loop cycle
}
