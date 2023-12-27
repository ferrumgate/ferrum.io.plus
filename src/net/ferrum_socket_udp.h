#ifndef __FERRUM_SOCKET_UDP_H__
#define __FERRUM_SOCKET_UDP_H__

#include "../common/common.h"
#include "../error/base_exception.h"
#include "../error/result.h"
#include "ferrum_addr.h"
#include "ferrum_socket.h"

namespace ferrum::io::net {

  class FerrumSocketUdp {
   public:
    using Ptr = std::shared_ptr<FerrumSocketUdp>;
    // callbacks for all typeof sockets
    using CallbackOnOpen = void(FerrumShared::Ptr &) noexcept;
    using CallbackOnRead = void(FerrumShared::Ptr &, const BufferByte &data,
                                FerrumAddr &&addr) noexcept;
    using CallbackOnWrite = void(FerrumShared::Ptr &) noexcept;
    using CallbackOnClose = void(FerrumShared::Ptr &) noexcept;
    using CallbackOnError = void(FerrumShared::Ptr &,
                                 error::BaseException) noexcept;

   public:
    FerrumSocketUdp(FerrumAddr &&addr, bool is_server = false);
    FerrumSocketUdp(FerrumSocketUdp &&socket);
    FerrumSocketUdp &operator=(FerrumSocketUdp &&socket);
    FerrumSocketUdp(const FerrumSocketUdp &socket) = delete;
    FerrumSocketUdp &operator=(const FerrumSocketUdp &socket) = delete;
    virtual ~FerrumSocketUdp();

    virtual void open(const FerrumSocketOptions &options);
    virtual void close() noexcept;
    virtual void write(const BufferByte &data, const FerrumAddr &addr);
    virtual void on_open(CallbackOnOpen func) noexcept;
    virtual void on_read(CallbackOnRead func) noexcept;
    virtual void on_write(CallbackOnWrite func) noexcept;
    virtual void on_close(CallbackOnClose func) noexcept;
    virtual void on_error(CallbackOnError func) noexcept;

    virtual void share(FerrumShared::Ptr shared) noexcept;
    virtual void bind(const FerrumAddr &addr);

    const FerrumAddr &addr() const;
    const FerrumAddr &bind_addr() const;

   protected:
    struct Socket {
      FerrumAddr addr;
      FerrumAddr bind_addr;
      bool is_server{false};
      CallbackOnOpen *callback_on_open{nullptr};
      CallbackOnRead *callback_on_read{nullptr};
      CallbackOnWrite *callback_on_write{nullptr};
      CallbackOnClose *callback_on_close{nullptr};
      CallbackOnError *callback_on_error{nullptr};

      // libuv fields
      uv_udp_t udp_data;
      uv_connect_t connect_data;
      // buffer for libuv read data
      BufferByte read_buffer;
      bool is_close_called{false};
      bool is_open_called{false};
      FerrumShared::Ptr shared;
    };
    Socket *socket{nullptr};

   public:  // friend functions
    friend void udp_socket_on_connect(uv_connect_t *connection, int status);
    friend void udp_socket_on_memory_alloc(uv_handle_t *client,
                                           size_t suggested_size,
                                           uv_buf_t *buf);
    friend void udp_socket_on_read(uv_udp_t *handle, ssize_t nread,
                                   const uv_buf_t *rcvbuf,
                                   const struct sockaddr *addr, unsigned flags);
    friend void udp_socket_on_send(uv_udp_send_t *req, int status);
    friend void udp_socket_on_close(uv_handle_t *handle);
    friend void udp_socket_on_accept(uv_stream_t *, int);
  };

}  // namespace ferrum::io::net

#endif