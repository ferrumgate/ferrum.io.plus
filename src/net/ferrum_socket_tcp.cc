#include "ferrum_socket_tcp.h"

namespace ferrum::io::net {

  using namespace ferrum::io;

  void tcp_socket_on_memory_alloc(uv_handle_t *client, size_t suggested_size,
                                  uv_buf_t *buf) {
    if(suggested_size <= 0) {
      log::Logger::warn("socket suggested_size is <0");
      return;
    }

    auto socket = static_cast<FerrumSocketTcp::Socket *>(client->data);
    if(socket->read_buffer.capacity() >= suggested_size) return;

    log::Logger::debug(
        std::format("tcp socket memory allocation {}", suggested_size));
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

  void tcp_socket_on_read(uv_stream_t *handle, ssize_t nread,
                          const uv_buf_t *rcvbuf) {
    log::Logger::debug(
        std::format("socket on recv called readsize: {}", nread));
    auto socket = static_cast<FerrumSocketTcp::Socket *>(handle->data);
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
        socket->callback_on_read(socket->shared, socket->read_buffer);
      }
    }
  }

  void tcp_socket_on_connect(uv_connect_t *connection, int status) {
    auto socket = static_cast<FerrumSocketTcp::Socket *>(connection->data);

    if(socket) {
      if(status < 0) {
        if(socket->callback_on_error) {
          socket->callback_on_error(
              socket->shared,
              error::BaseException(
                  common::ErrorCodes::SocketError,
                  std::format("connection failed with error code: {} msg: {}",
                              status, uv_strerror(status))));
        }
      } else {
        uv_stream_t *stream =
            reinterpret_cast<uv_stream_t *>(&socket->tcp_data);
        auto result = common::FuncTable::uv_read_start(
            stream, tcp_socket_on_memory_alloc, tcp_socket_on_read);
        if(result) {
          log::Logger::error(
              std::format("tcp socket read start failed code: {} msg: {}",
                          result, uv_strerror(result)));
          if(socket->callback_on_error) {
            socket->callback_on_error(
                socket->shared,
                error::BaseException(
                    common::ErrorCodes::SocketError,
                    std::format("tcp socket open read start code: {} msg: {}",
                                result, uv_strerror(result))));
          }
        } else if(socket->callback_on_open) {
          socket->callback_on_open(socket->shared);
        }
      }
    }
  }
  void tcp_socket_on_send(uv_write_t *req, int status) {
    log::Logger::debug(
        std::format("socket on send called and status: {}", status));

    if(req->handle &&
       !uv_is_closing(reinterpret_cast<uv_handle_t *>(req->handle)) &&
       req->handle->data) {
      auto socket = static_cast<FerrumSocketTcp::Socket *>(req->handle->data);

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

  void tcp_socket_on_close(uv_handle_t *handle) {
    if(handle)
      if(handle->data && uv_is_closing(handle)) {
        auto socket = static_cast<FerrumSocketTcp::Socket *>(handle->data);
        handle->data = nullptr;
        if(socket->is_open_called && socket->callback_on_close) {
          log::Logger::debug("handle closed");
          socket->callback_on_close(socket->shared);
        }
        delete socket;
      }
  }

  void tcp_socket_on_accept(uv_stream_t *server, int status) {
    auto socket = static_cast<FerrumSocketTcp::Socket *>(server->data);
    if(status < 0) {
      log::Logger::debug(
          std::format("error on new accept {}", uv_strerror(status)));
      if(socket && socket->callback_on_error) {
        socket->callback_on_error(
            socket->shared,
            error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("error no new accept 1 {}", uv_strerror(status))));
      }
      return;
    }
    auto client = std::make_shared<FerrumSocketTcp>(
        FerrumSocketTcp{FerrumAddr{"0.0.0.0"}});

    auto result = uv_accept(
        server, reinterpret_cast<uv_stream_t *>(&client->socket->tcp_data));
    if(result < 0) {
      if(socket && socket->callback_on_error) {
        socket->callback_on_error(
            socket->shared,
            error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("error no new accept 2 {}", uv_strerror(status))));
      }
      return;
    }
    // get client ip and port
    int addr_len = sizeof(sockaddr_storage);
    auto addr = sockaddr_storage{};

    // no need to check it
    uv_tcp_getpeername(&client->socket->tcp_data,
                       reinterpret_cast<sockaddr *>(&addr), &addr_len);
    auto faddr = FerrumAddr{reinterpret_cast<const sockaddr *>(&addr)};
    client->socket->addr = faddr;
    client->socket->is_open_called = true;
    log::Logger::info(std::format("connected client from {}",
                                  client->socket->addr.to_string(true)));

    uv_stream_t *stream =
        reinterpret_cast<uv_stream_t *>(&client->socket->tcp_data);
    result = common::FuncTable::uv_read_start(
        stream, tcp_socket_on_memory_alloc, tcp_socket_on_read);
    if(result) {
      log::Logger::error(
          std::format("tcp socket read start failed code: {} msg: {}", result,
                      uv_strerror(result)));
      if(socket->callback_on_error) {
        socket->callback_on_error(
            socket->shared,
            error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket open read start code: {} msg: {}",
                            result, uv_strerror(result))));
      }
    } else if(socket->callback_on_accept) {
      auto client_casted = std::static_pointer_cast<FerrumSocket>(client);
      socket->callback_on_accept(socket->shared, client_casted);
    }
  }

  FerrumSocketTcp::FerrumSocketTcp(FerrumAddr &&addr, bool is_server)
      : FerrumSocket{},
        socket{new Socket{is_server ? FerrumAddr{"0.0.0.0"} : addr,
                          is_server ? addr : FerrumAddr{"0.0.0.0"},
                          is_server}} {
    auto loop = uv_default_loop();
    auto result = common::FuncTable::uv_tcp_init(loop, &socket->tcp_data);
    if(result < 0) {
      throw error::BaseException(
          common::ErrorCodes::SocketError,
          std::format("tcp socket create failed {}", uv_strerror(result)));
    }
    uv_tcp_keepalive(&socket->tcp_data, 1, 60);
    socket->tcp_data.data = socket;
  }

  FerrumSocketTcp::FerrumSocketTcp(FerrumSocketTcp &&other)
      : FerrumSocket{std::move(other)},
        socket(std::move(other.socket))

  {
    other.socket = nullptr;
  }
  FerrumSocketTcp &FerrumSocketTcp::operator=(FerrumSocketTcp &&other) {
    FerrumSocket::operator=(std::move(other));
    delete socket;
    socket = std::move(other.socket);
    other.socket = nullptr;
    return *this;
  }
  FerrumSocketTcp::~FerrumSocketTcp() {
    close();
    FerrumSocket::~FerrumSocket();
  }

  void FerrumSocketTcp::open(const FerrumSocketOptions &options) {
    if(socket->is_open_called) return;
    if(!socket->is_server) {
      socket->connect_data.data = socket;
      auto result = common::FuncTable::uv_tcp_connect(
          &socket->connect_data, &socket->tcp_data, socket->addr.get_addr(),
          tcp_socket_on_connect);
      if(result < 0) {
        throw error::BaseException(
            common::ErrorCodes::SocketError,
            std::format("tcp socket open failed code: {} msg: {}", result,
                        uv_strerror(result)));
      }
    } else {
      bind(socket->bind_addr);
      // if linux,then set reuse port option to socket
      auto saddr = socket->bind_addr.get_addr();
      if(saddr->sa_family == AF_INET || saddr->sa_family == AF_INET6) {
        uv_os_fd_t fd;
        auto result = common::FuncTable::uv_fileno(
            reinterpret_cast<uv_handle_t *>(&socket->tcp_data), &fd);
        if(result < 0) {
          throw error::BaseException(
              common::ErrorCodes::SocketError,
              std::format("tcp socket open failed code: {} msg: {}", result,
                          uv_strerror(result)));
        }
        int32_t optval = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
      }

      auto result = common::FuncTable::uv_listen(
          reinterpret_cast<uv_stream_t *>(&socket->tcp_data), 16,
          tcp_socket_on_accept);
      if(result < 0) {
        throw error::BaseException(
            common::ErrorCodes::SocketError,
            std::format("tcp socket open failed code: {} msg: {}", result,
                        uv_strerror(result)));
      }
      log::Logger::info(std::format("socket started to listen at {}",
                                    socket->bind_addr.to_string(true)));
    }

    socket->is_open_called = true;
  }

  void FerrumSocketTcp::bind(const FerrumAddr &addr) {
    socket->bind_addr = addr;
    auto bind_addr6 = socket->bind_addr.get_addr();
    auto result =
        common::FuncTable::uv_tcp_bind(&socket->tcp_data, bind_addr6, 0);
    if(result < 0) {
      throw error::BaseException(
          common::ErrorCodes::SocketError,
          std::format("tcp socket create failed {}", uv_strerror(result)));
    }
  }

  void FerrumSocketTcp::close() noexcept {
    if(!socket || socket->is_close_called) return;
    socket->is_close_called = true;
    uv_handle_t *handle = reinterpret_cast<uv_handle_t *>(&socket->tcp_data);
    if(!uv_is_closing(handle)) {
      log::Logger::info(std::format(" closing(reset) connection {}",
                                    socket->addr.to_string(true)));
      uv_tcp_close_reset(&socket->tcp_data, tcp_socket_on_close);
    }
  }
  void FerrumSocketTcp::write(const BufferByte &data) {
    if(uv_is_closing(reinterpret_cast<uv_handle_t *>(&socket->tcp_data))) {
      throw error::BaseException(common::ErrorCodes::SocketError,
                                 std::format("socket is closing"));
    }

    // std::make_unique<uv_write_t>(uv_write_t{});
    auto buf_ptr = data.clone_ptr();  // this must be first, if exception
                                      // occures memory safety
    auto request = new uv_write_t;
    request->data = buf_ptr;

    uv_buf_t buf = uv_buf_init(reinterpret_cast<char *>(buf_ptr), data.size());

    auto result = common::FuncTable::uv_write(
        request, reinterpret_cast<uv_stream_t *>(&socket->tcp_data), &buf, 1,
        tcp_socket_on_send);
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
  void FerrumSocketTcp::on_open(CallbackOnOpen func) noexcept {
    socket->callback_on_open = func;
  }
  void FerrumSocketTcp::on_read(CallbackOnRead func) noexcept {
    socket->callback_on_read = func;
  }
  void FerrumSocketTcp::on_write(CallbackOnWrite func) noexcept {
    socket->callback_on_write = func;
  }
  void FerrumSocketTcp::on_close(CallbackOnClose func) noexcept {
    socket->callback_on_close = func;
  }
  void FerrumSocketTcp::on_error(CallbackOnError func) noexcept {
    socket->callback_on_error = func;
  }
  void FerrumSocketTcp::on_accept(CallbackOnAccept func) noexcept {
    socket->callback_on_accept = func;
  }
  void FerrumSocketTcp::share(Shared shared) noexcept {
    socket->shared = shared;
  }
}  // namespace ferrum::io::net