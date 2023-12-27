#include "ferrum_socket_udp.h"

namespace ferrum::io::net {

  using namespace ferrum::io;

  void udp_socket_on_memory_alloc(uv_handle_t *client, size_t suggested_size,
                                  uv_buf_t *buf) {
    if(suggested_size <= 0) {
      log::Logger::warn("socket suggested_size is <0");
      return;
    }

    auto socket = static_cast<FerrumSocketUdp::Socket *>(client->data);
    if(socket->read_buffer.capacity() >= suggested_size) return;

    log::Logger::debug(
        std::format("udp socket memory allocation {}", suggested_size));
    auto result = socket->read_buffer.reserve_noexcept(suggested_size);
    if(result) {
      log::Logger::fatal("memory allocation failed");
      std::exit(1);
    }
    buf->base = reinterpret_cast<char *>(socket->read_buffer.array_ptr());
    buf->len = suggested_size;
#ifndef NDEBUG
    std::memset(buf->base, 0, buf->len);
#endif
  }

  void udp_socket_on_read(uv_udp_t *handle, ssize_t nread,
                          const uv_buf_t *rcvbuf, const struct sockaddr *addr,
                          unsigned flags) {
    log::Logger::debug(
        std::format("socket on recv called readsize: {}", nread));
    auto socket = static_cast<FerrumSocketUdp::Socket *>(handle->data);
    if(!uv_is_closing(reinterpret_cast<uv_handle_t *>(handle))) {
      if(nread < 0) {
        if(socket->callback_on_error) {
          log::Logger::debug(
              std::format("socket error occured code: {} msg: {}\n", nread,
                          uv_strerror(nread)));
          socket->callback_on_error(
              socket->shared,
              error::BaseException(nread == UV_EOF
                                       ? common::ErrorCodes::SocketClosedError
                                       : common::ErrorCodes::SocketError,
                                   std::format("socket error code: {} msg: {}",
                                               nread, uv_strerror(nread))));
        }
      } else if(socket->callback_on_read && nread > 0) {
        log::Logger::debug(std::format("socket receive nread:{} buflen:{}\n",
                                       nread, rcvbuf->len));
        socket->read_buffer.resize(nread);

        socket->callback_on_read(socket->shared, socket->read_buffer,
                                 FerrumAddr{addr, false});
      }
    }
  }

  void udp_socket_on_send(uv_udp_send_t *req, int status) {
    log::Logger::debug(
        std::format("socket on send called and status: {}", status));

    if(req->handle &&
       !uv_is_closing(reinterpret_cast<uv_handle_t *>(req->handle)) &&
       req->handle->data) {
      auto socket = static_cast<FerrumSocketUdp::Socket *>(req->handle->data);

      if(status < 0) {
        if(socket->callback_on_error)
          socket->callback_on_error(
              socket->shared,
              error::BaseException(
                  common::ErrorCodes::SocketError,
                  std::format("socket write failed errcode: {} {}", status,
                              uv_strerror(status))));
      } else if(socket->callback_on_write) {
        socket->callback_on_write(socket->shared);
      }
    }
    if(req->data) delete[] reinterpret_cast<std::byte *>(req->data);
    delete req;
  }

  void udp_socket_on_close(uv_handle_t *handle) {
    if(handle)
      if(handle->data && uv_is_closing(handle)) {
        auto socket = static_cast<FerrumSocketUdp::Socket *>(handle->data);
        handle->data = nullptr;
        if(socket->is_open_called && socket->callback_on_close) {
          log::Logger::debug("handle closed");
          socket->callback_on_close(socket->shared);
        }
        delete socket;
      }
  }

  FerrumSocketUdp::FerrumSocketUdp(FerrumAddr &&addr, bool is_server)
      : socket{new Socket{is_server ? FerrumAddr{"0.0.0.0"} : addr,
                          is_server ? addr : FerrumAddr{"0.0.0.0"},
                          is_server}} {
    auto loop = uv_default_loop();
    auto result = common::FuncTable::uv_udp_init(loop, &socket->udp_data);
    if(result < 0) {
      throw error::BaseException(
          common::ErrorCodes::SocketError,
          std::format("udp socket create failed {}", uv_strerror(result)));
    }
    socket->udp_data.data = socket;
  }

  FerrumSocketUdp::FerrumSocketUdp(FerrumSocketUdp &&other)
      : socket(std::move(other.socket))

  {
    other.socket = nullptr;
  }
  FerrumSocketUdp &FerrumSocketUdp::operator=(FerrumSocketUdp &&other) {
    delete socket;
    socket = std::move(other.socket);
    other.socket = nullptr;
    return *this;
  }
  FerrumSocketUdp::~FerrumSocketUdp() { close(); }

  void FerrumSocketUdp::open(const FerrumSocketOptions &options) {
    if(socket->is_open_called) return;
    if(!socket->is_server) {
      socket->connect_data.data = socket;
      // if socket addr

    } else {
      bind(socket->bind_addr);

      log::Logger::info(std::format("socket started to listen at {}",
                                    socket->bind_addr.to_string(true)));
    }
    socket->is_open_called = true;
    // uv_stream_t *stream = reinterpret_cast<uv_stream_t *>(&socket->udp_data);
    auto result = common::FuncTable::uv_udp_read_start(
        &socket->udp_data, udp_socket_on_memory_alloc, udp_socket_on_read);
    if(result) {
      log::Logger::error(
          std::format("udp socket read start failed code: {} msg: {}", result,
                      uv_strerror(result)));

      throw error::BaseException(
          common::ErrorCodes::SocketError,
          std::format("udp socket open read start code: {} msg: {}", result,
                      uv_strerror(result)));
    }

    if(socket->callback_on_open) {
      socket->callback_on_open(socket->shared);
    }
  }

  void FerrumSocketUdp::bind(const FerrumAddr &addr) {
    socket->bind_addr = addr;
    auto bind_addr6 = socket->bind_addr.get_addr();
    auto result = common::FuncTable::uv_udp_bind(&socket->udp_data, bind_addr6,
                                                 UV_UDP_REUSEADDR);
    if(result < 0) {
      throw error::BaseException(
          common::ErrorCodes::SocketError,
          std::format("udp socket create failed {}", uv_strerror(result)));
    }
  }

  void FerrumSocketUdp::close() noexcept {
    if(!socket || socket->is_close_called) return;
    socket->is_close_called = true;
    uv_handle_t *handle = reinterpret_cast<uv_handle_t *>(&socket->udp_data);
    if(!uv_is_closing(handle)) {
      log::Logger::info(
          std::format(" closing connection {}", socket->addr.to_string(true)));
      uv_close(handle, udp_socket_on_close);
    }
  }
  void FerrumSocketUdp::write(const BufferByte &data, const FerrumAddr &addr) {
    if(uv_is_closing(reinterpret_cast<uv_handle_t *>(&socket->udp_data))) {
      throw error::BaseException(common::ErrorCodes::SocketError,
                                 std::format("socket is closing"));
    }

    // std::make_unique<uv_write_t>(uv_write_t{});
    auto buf_ptr = data.clone_ptr();  // this must be first, if exception
                                      // occures memory safety
    auto request = new uv_udp_send_t;
    request->data = buf_ptr;

    uv_buf_t buf = uv_buf_init(reinterpret_cast<char *>(buf_ptr), data.size());

    auto result =
        common::FuncTable::uv_udp_write(request, &socket->udp_data, &buf, 1,
                                        addr.get_addr(), udp_socket_on_send);
    if(result < 0) {
      delete[] buf_ptr;
      delete request;
      log::Logger::debug(std::format("sending data to failed: %s",
                                     socket->addr.to_string(true)));
      throw error::BaseException(common::ErrorCodes::SocketError,
                                 std::format("sending data failed to {}",
                                             socket->addr.to_string(true)));
    }
  }
  void FerrumSocketUdp::on_open(CallbackOnOpen func) noexcept {
    socket->callback_on_open = func;
  }
  void FerrumSocketUdp::on_read(CallbackOnRead func) noexcept {
    socket->callback_on_read = func;
  }
  void FerrumSocketUdp::on_write(CallbackOnWrite func) noexcept {
    socket->callback_on_write = func;
  }
  void FerrumSocketUdp::on_close(CallbackOnClose func) noexcept {
    socket->callback_on_close = func;
  }
  void FerrumSocketUdp::on_error(CallbackOnError func) noexcept {
    socket->callback_on_error = func;
  }

  void FerrumSocketUdp::share(FerrumShared::Ptr shared) noexcept {
    socket->shared = shared;
  }

  const FerrumAddr &FerrumSocketUdp::addr() const { return socket->addr; }
  const FerrumAddr &FerrumSocketUdp::bind_addr() const {
    return socket->bind_addr;
  }
}  // namespace ferrum::io::net