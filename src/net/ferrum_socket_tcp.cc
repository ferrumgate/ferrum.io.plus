#include "ferrum_socket_tcp.h"

namespace ferrum::io::net
{

    using namespace ferrum::io;

    void socket_on_connect(uv_connect_t *connection, int status)
    {

        auto socket = static_cast<FerrumSocketTcp *>(connection->data);

        if (socket)
        {
            if (status < 0)
            {
                if (socket->callback_on_error)
                {
                    socket->callback_on_error(
                        error::BaseException(
                            common::ErrorCodes::SocketError,
                            std::format("connection failed with error code: {} msg: {}", status, uv_strerror(status))));
                }
            }
            else if (socket->callback_on_open)
            {
                socket->callback_on_open();
            }
        }
    }

    void socket_on_memory_alloc(uv_handle_t *client, size_t suggested_size, uv_buf_t *buf)
    {
        if (suggested_size <= 0)
        {
            log::Logger::warn("socket suggested_size is <0");
            return;
        }

        auto socket = static_cast<FerrumSocketTcp *>(client->data);
        if (socket->read_buffer.capacity() >= suggested_size)
            return;

        log::Logger::debug(std::format("tcp socket memory allocation {}", suggested_size));
        auto result = socket->read_buffer.reserve_noexcept(suggested_size);
        if (result)
        {
            log::Logger::fatal("memory allocation failed");
            std::exit(1);
        }
        buf->base = reinterpret_cast<char *>(socket->read_buffer.array_ptr());
        buf->len = suggested_size;
#ifndef NDEBUG
        std::memset(buf->base, 0, buf->len);
#endif
    }

    void socket_on_read(uv_stream_t *handle, ssize_t nread, const uv_buf_t *rcvbuf)
    {
        log::Logger::debug(std::format("socket on recv called readsize: {}", nread));
        auto socket = static_cast<FerrumSocketTcp *>(handle->data);
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(handle)))
        {
            if (nread < 0)
            {
                if (socket->callback_on_error)
                {
                    log::Logger::debug(std::format("socket error occured {}\n", nread));
                    socket->callback_on_error(error::BaseException(
                        common::ErrorCodes::SocketError,
                        std::format("socker error code: {} msg: {}", nread, strerror(nread))));
                }
            }
            else if (socket->callback_on_read && nread > 0)
            {

                log::Logger::debug(std::format("socket receive nread:{} buflen:{}\n", nread, rcvbuf->len));
                socket->read_buffer.resize(nread);
                socket->callback_on_read(socket->read_buffer);
            }
        }
    }

    void socket_on_send(uv_write_t *req, int status)
    {

        log::Logger::debug(std::format("socket on send called and status: {}", status));

        if (req->handle && !uv_is_closing(reinterpret_cast<uv_handle_t *>(req->handle)) && req->handle->data)
        {
            auto socket = static_cast<FerrumSocketTcp *>(req->handle->data);

            if (status < 0)
            {
                if (socket->callback_on_error)
                    socket->callback_on_error(
                        error::BaseException(
                            common::ErrorCodes::SocketError,
                            std::format("socket write failed errcode: {} {}", status, uv_err_name(status))));
            }
            else if (socket->callback_on_write)
            {
                //  socket->callback_on_write(cast_to_socket(socket), socket->callback_data, source);
            }
        }
    }

    void socket_on_close(uv_handle_t *handle)
    {

        if (handle)
            if (handle->data && uv_is_closing(handle))
            {
                auto socket = static_cast<FerrumSocketTcp *>(handle->data);
                handle->data = nullptr;
                if (socket->callback_on_close)
                {
                    log::Logger::debug("handle closed");
                    socket->callback_on_close();
                }
            }
    }

    FerrumSocketTcp::FerrumSocketTcp(FerrumAddr &&addr)
        : FerrumSocket{}, addr{addr}, callback_on_open{}, callback_on_read{}, callback_on_write{},
          callback_on_close{}, callback_on_error{}, read_buffer{0}
    {
        auto loop = uv_default_loop();
        auto result = uv_tcp_init(loop, &tcp_data);
        if (result < 0)
        {
            throw error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket create failed {}", uv_strerror(result)));
        }
        uv_tcp_keepalive(&tcp_data, 1, 60);

        auto bind_addr = FerrumAddr("::");
        auto bind_addr6 = bind_addr.get_addr6();
        result = uv_tcp_bind(&tcp_data, reinterpret_cast<sockaddr *>(&bind_addr6), 0);
        if (result < 0)
        {
            throw error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket create failed {}", uv_strerror(result)));
        }
        tcp_data.data = this;
    }

    FerrumSocketTcp::FerrumSocketTcp(FerrumSocketTcp &&other)
        : FerrumSocket{std::move(other)}, addr{other.addr},
          callback_on_open{std::move(other.callback_on_open)},
          callback_on_read{std::move(other.callback_on_read)},
          callback_on_write{std::move(other.callback_on_write)},
          callback_on_close{std::move(other.callback_on_close)},
          callback_on_error{std::move(other.callback_on_error)},
          tcp_data{std::move(other.tcp_data)},
          connect_data{std::move(other.connect_data)},
          read_buffer{std::move(other.read_buffer)}

    {
    }
    FerrumSocketTcp &FerrumSocketTcp::operator=(FerrumSocketTcp &&other)
    {
        FerrumSocket::operator=(std::move(other));
        addr = other.addr;
        callback_on_open = std::move(other.callback_on_open);
        callback_on_read = std::move(other.callback_on_read);
        callback_on_write = std::move(other.callback_on_write);
        callback_on_close = std::move(other.callback_on_close);
        callback_on_error = std::move(other.callback_on_error);
        tcp_data = std::move(other.tcp_data);
        connect_data = std::move(other.connect_data);
        read_buffer = std::move(other.read_buffer);
        return *this;
    }
    FerrumSocketTcp::~FerrumSocketTcp()
    {

        FerrumSocket::~FerrumSocket();
    }

    void FerrumSocketTcp::open()
    {
        if (is_open_called)
            return;
        auto result = uv_tcp_connect(&connect_data, &tcp_data, addr.get_addr(), socket_on_connect);
        if (result < 0)
        {
            throw error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket open failed code: {} msg: {}", result, uv_strerror(result)));
        }
        uv_stream_t *stream = reinterpret_cast<uv_stream_t *>(&tcp_data);
        result = uv_read_start(stream, socket_on_memory_alloc, socket_on_read);
        if (result)
        {
            throw error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket open failed code: {} msg: {}", result, uv_strerror(result)));
        }
        is_open_called = true;
    }
    void FerrumSocketTcp::close()
    {
        if (is_close_called)
            return;
        is_close_called = true;
        uv_handle_t *handle = reinterpret_cast<uv_handle_t *>(&tcp_data);
        if (!uv_is_closing(handle))
        {

            log::Logger::info(std::format(" closing(reset) connection {}", addr.to_string(true)));
            uv_tcp_close_reset(&tcp_data, socket_on_close);
        }
    }
    void FerrumSocketTcp::write(const BufferByte &data)
    {
        if (uv_is_closing(reinterpret_cast<uv_handle_t *>(&tcp_data)))
        {
            throw error::BaseException(common::ErrorCodes::SocketError, std::format("socket is closing"));
        }

        uv_write_t *request = new uv_write_t;
        auto buf_ptr = data.clone_ptr();
        request->data = buf_ptr;
        uv_buf_t buf = uv_buf_init(reinterpret_cast<char *>(buf_ptr), data.size());

        auto result = uv_write(request, reinterpret_cast<uv_stream_t *>(&tcp_data), &buf, 1, socket_on_send);
        if (result < 0)
        {
            log::Logger::debug(std::format("sending data to failed: %s", addr.to_string(true)));
            delete request;
            delete buf_ptr;
            throw error::BaseException(common::ErrorCodes::SocketError, std::format("sending data failed to {}", addr.to_string(true)));
        }
    }
    void FerrumSocketTcp::on_open(CallbackOnOpen &func)
    {
        callback_on_open = func;
    }
    void FerrumSocketTcp::on_read(CallbackOnRead &func)
    {
        callback_on_read = func;
    }
    void FerrumSocketTcp::on_write(CallbackOnWrite &func)
    {
        callback_on_write = func;
    }
    void FerrumSocketTcp::on_close(CallbackOnClose &func)
    {
        callback_on_close = func;
    }
    void FerrumSocketTcp::on_error(CallbackOnError &func)
    {
        callback_on_error = func;
    }
}