#ifndef __FERRUM_SOCKET_H__
#define __FERRUM_SOCKET_H__

#include "../common/common.h"
#include "ferrum_addr.h"
#include "../error/result.h"
#include "../error/base_exception.h"
#include "../log/logger.h"
#include "../memory/buffer.h"

namespace ferrum::io::net
{

    struct FerrumShared
    {
    };

    // zero copy buffer
    using BufferByte = memory::Buffer<std::byte>; //  std::vector<std::byte>;
    using Shared = std::shared_ptr<FerrumShared>;

    // callbacks
    using CallbackOnOpen = void(Shared &);
    using CallbackOnRead = void(Shared &, const BufferByte &data);
    using CallbackOnWrite = void(Shared &);
    using CallbackOnClose = void(Shared &);
    using CallbackOnError = void(Shared &, error::BaseException);

    class FerrumSocket
    {
    public:
        FerrumSocket() = default;
        FerrumSocket(const FerrumSocket &) = delete;
        FerrumSocket(FerrumSocket &&) = default;
        FerrumSocket &operator=(const FerrumSocket &) = delete;
        FerrumSocket &operator=(FerrumSocket &&) = default;
        virtual ~FerrumSocket() = default;

        virtual void share(Shared shared) = 0;
        virtual void bind(const FerrumAddr &addr) = 0;

        virtual void open() = 0;
        virtual void close() = 0;
        virtual void write(const BufferByte &data) = 0;
        virtual void on_open(CallbackOnOpen func) = 0;
        virtual void on_read(CallbackOnRead func) = 0;
        virtual void on_write(CallbackOnWrite func) = 0;
        virtual void on_close(CallbackOnClose func) = 0;
        virtual void on_error(CallbackOnError func) = 0;

    protected:
    };

    using FerrumSocketPtr = std::unique_ptr<FerrumSocket>;
    using CallbackOnAccept = void(FerrumSocket &&client);

    class FerrumSocketListener
    {
    public:
        virtual void listen() = 0;
        virtual void on_accept(CallbackOnAccept func) = 0;
        virtual void close() = 0;
        virtual void on_close(CallbackOnClose func) = 0;
        virtual void on_error(CallbackOnError &func);
    };

}

#endif