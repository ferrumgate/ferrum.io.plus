#ifndef __FERRUM_SOCKET_TCP_H__
#define __FERRUM_SOCKET_TCP_H__

#include "../common/common.h"
#include "ferrum_addr.h"
#include "../error/result.h"
#include "../error/base_exception.h"
#include "ferrum_socket.h"

namespace ferrum::io::net
{

    class FerrumSocketTcp : public FerrumSocket
    {
    public:
        FerrumSocketTcp(FerrumAddr &&addr);
        FerrumSocketTcp(FerrumSocketTcp &&socket);
        FerrumSocketTcp &operator=(FerrumSocketTcp &&socket);
        FerrumSocketTcp(const FerrumSocketTcp &socket) = delete;
        FerrumSocketTcp &operator=(const FerrumSocketTcp &socket) = delete;
        virtual ~FerrumSocketTcp();

        virtual void open() override;
        virtual void close() override;
        virtual void write(const BufferByte &data) override;
        virtual void on_open(CallbackOnOpen &func) override;
        virtual void on_read(CallbackOnRead &func) override;
        virtual void on_write(CallbackOnWrite &func) override;
        virtual void on_close(CallbackOnClose &func) override;
        virtual void on_error(CallbackOnError &func) override;

    protected:
        struct Socket
        {
            FerrumAddr addr;
            CallbackOnOpen callback_on_open;
            CallbackOnRead callback_on_read;
            CallbackOnWrite callback_on_write;
            CallbackOnClose callback_on_close;
            CallbackOnError callback_on_error;
            // libuv fields
            uv_tcp_t tcp_data;
            uv_connect_t connect_data;
            // buffer for libuv read data
            BufferByte read_buffer;
            bool is_close_called{false};
            bool is_open_called{false};
        };
        Socket *socket{nullptr};

    public: // friend functions
        friend void socket_on_connect(uv_connect_t *connection, int status);
        friend void socket_on_memory_alloc(uv_handle_t *client, size_t suggested_size, uv_buf_t *buf);
        friend void socket_on_read(uv_stream_t *handle, ssize_t nread, const uv_buf_t *rcvbuf);
        friend void socket_on_send(uv_write_t *req, int status);
        friend void socket_on_close(uv_handle_t *handle);
    };

}

#endif