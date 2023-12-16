#include "ferrum_socket_tcp.h"

namespace ferrum::io::net
{

    using namespace ferrum::io;

    void socket_on_memory_alloc(uv_handle_t *client, size_t suggested_size, uv_buf_t *buf)
    {
        if (suggested_size <= 0)
        {
            log::Logger::warn("socket suggested_size is <0");
            return;
        }

        auto socket = static_cast<FerrumSocketTcp::Socket *>(client->data);
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
        auto socket = static_cast<FerrumSocketTcp::Socket *>(handle->data);
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(handle)))
        {
            if (nread < 0)
            {
                if (socket->callback_on_error)
                {
                    log::Logger::debug(std::format("socket error occured {}\n", nread));
                    socket->callback_on_error(error::BaseException(
                        nread == UV_EOF ? common::ErrorCodes::SocketClosedError : common::ErrorCodes::SocketError,
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

    void socket_on_connect(uv_connect_t *connection, int status)
    {

        auto socket = static_cast<FerrumSocketTcp::Socket *>(connection->data);

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
            else
            {
                uv_stream_t *stream = reinterpret_cast<uv_stream_t *>(&socket->tcp_data);
                auto result = uv_read_start(stream, socket_on_memory_alloc, socket_on_read);
                if (result)
                {
                    log::Logger::error(std::format("tcp socket read start failed code: {} msg: {}", result, uv_strerror(result)));
                    if (socket->callback_on_error)
                    {
                        socket->callback_on_error(error::BaseException(common::ErrorCodes::SocketError, std::format("tcp socket open read start code: {} msg: {}", result, uv_strerror(result))));
                    }
                }
                else if (socket->callback_on_open)
                {
                    socket->callback_on_open();
                }
            }
        }
    }
    void socket_on_send(uv_write_t *req, int status)
    {

        log::Logger::debug(std::format("socket on send called and status: {}", status));

        if (req->handle && !uv_is_closing(reinterpret_cast<uv_handle_t *>(req->handle)) && req->handle->data)
        {
            auto socket = static_cast<FerrumSocketTcp::Socket *>(req->handle->data);

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
                socket->callback_on_write();
            }
        }
        if (req->handle->data)
            delete[] reinterpret_cast<std::byte *>(req->handle->data);
        delete req;
    }

    void socket_on_close(uv_handle_t *handle)
    {

        if (handle)
            if (handle->data && uv_is_closing(handle))
            {
                auto socket = static_cast<FerrumSocketTcp::Socket *>(handle->data);
                handle->data = nullptr;
                if (socket->is_open_called && socket->callback_on_close)
                {
                    log::Logger::debug("handle closed");
                    socket->callback_on_close();
                }
                delete socket;
            }
    }

    FerrumSocketTcp::FerrumSocketTcp(FerrumAddr &&addr)
        : FerrumSocket{}, socket{new Socket{addr}}
    {

        auto loop = uv_default_loop();
        auto result = common::FuncTable::uv_tcp_init(loop, &socket->tcp_data);
        if (result < 0)
        {
            throw error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket create failed {}", uv_strerror(result)));
        }
        uv_tcp_keepalive(&socket->tcp_data, 1, 60);

        auto bind_addr = FerrumAddr("::");
        auto bind_addr6 = bind_addr.get_addr();
        result = common::FuncTable::uv_tcp_bind(&socket->tcp_data, bind_addr6, 0);
        if (result < 0)
        {
            throw error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket create failed {}", uv_strerror(result)));
        }
        socket->tcp_data.data = socket;
    }

    FerrumSocketTcp::FerrumSocketTcp(FerrumSocketTcp &&other)
        : FerrumSocket{std::move(other)}, socket(std::move(other.socket))

    {
        other.socket = nullptr;
    }
    FerrumSocketTcp &FerrumSocketTcp::operator=(FerrumSocketTcp &&other)
    {
        FerrumSocket::operator=(std::move(other));
        delete socket;
        socket = std::move(other.socket);
        other.socket = nullptr;
        return *this;
    }
    FerrumSocketTcp::~FerrumSocketTcp()
    {
        close();
        FerrumSocket::~FerrumSocket();
    }

    void FerrumSocketTcp::open()
    {
        if (socket->is_open_called)
            return;
        auto result = common::FuncTable::uv_tcp_connect(&socket->connect_data, &socket->tcp_data, socket->addr.get_addr(), socket_on_connect);
        if (result < 0)
        {
            throw error::BaseException(
                common::ErrorCodes::SocketError,
                std::format("tcp socket open failed code: {} msg: {}", result, uv_strerror(result)));
        }

        socket->is_open_called = true;
    }
    void FerrumSocketTcp::close()
    {
        if (socket->is_close_called)
            return;
        socket->is_close_called = true;
        uv_handle_t *handle = reinterpret_cast<uv_handle_t *>(&socket->tcp_data);
        if (!uv_is_closing(handle))
        {

            log::Logger::info(std::format(" closing(reset) connection {}", socket->addr.to_string(true)));
            uv_tcp_close_reset(&socket->tcp_data, socket_on_close);
        }
    }
    void FerrumSocketTcp::write(const BufferByte &data)
    {
        if (uv_is_closing(reinterpret_cast<uv_handle_t *>(&socket->tcp_data)))
        {
            throw error::BaseException(common::ErrorCodes::SocketError, std::format("socket is closing"));
        }

        uv_write_t *request = new uv_write_t;
        auto buf_ptr = data.clone_ptr();
        request->data = buf_ptr;
        uv_buf_t buf = uv_buf_init(reinterpret_cast<char *>(buf_ptr), data.size());

        auto result = uv_write(request, reinterpret_cast<uv_stream_t *>(&socket->tcp_data), &buf, 1, socket_on_send);
        if (result < 0)
        {
            log::Logger::debug(std::format("sending data to failed: %s", socket->addr.to_string(true)));
            delete request;
            delete[] buf_ptr;
            throw error::BaseException(common::ErrorCodes::SocketError,
                                       std::format("sending data failed to {}", socket->addr.to_string(true)));
        }
    }
    void FerrumSocketTcp::on_open(CallbackOnOpen func)
    {
        socket->callback_on_open = func;
    }
    void FerrumSocketTcp::on_read(CallbackOnRead func)
    {
        socket->callback_on_read = func;
    }
    void FerrumSocketTcp::on_write(CallbackOnWrite func)
    {
        socket->callback_on_write = func;
    }
    void FerrumSocketTcp::on_close(CallbackOnClose func)
    {
        socket->callback_on_close = func;
    }
    void FerrumSocketTcp::on_error(CallbackOnError func)
    {
        socket->callback_on_error = func;
    }
}