#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "../common/common.h"
#include "../memory/buffer.h"
#include "../error/base_exception.h"
#include "../log/logger.h"

namespace ferrum::io::memory
{

    /**
     * @brief simple vector implementation but resize does not clear items. only sets len property
     *
     * @tparam T
     */
    template <typename T>
    class Buffer
    {
        using Allocator = std::function<std::shared_ptr<T[]>(size_t)>;

    public:
        static auto malloc_array(size_t len)
        {
            return std::shared_ptr<T[]>{new (std::nothrow) T[len]};
        };

        Buffer(size_t len = 0, Allocator malloc = Buffer<T>::malloc_array) : len{len},
                                                                             reserved{len}, data{nullptr}, malloc(malloc)
        {

            if (len)
            {
                data = std::move(malloc(len));
                if (!data)
                {
                    log::Logger::fatal("memory allocation failed");
                    throw error::BaseException(common::ErrorCodes::MemoryError, "memory allocation failed");
                }
            }
        };
        Buffer(Buffer &buffer) : data(new T[buffer.reserved]), len(buffer.len), reserved(buffer.reserved)
        {
            std::memcpy(data.get(), buffer.array_ptr(), len * sizeof(T));
        };
        Buffer &operator=(Buffer &buffer)
        {
            data = new T[buffer.reserved];
            std::memcpy(data, buffer.data, buffer.len);
            len = buffer.len;
            reserved = buffer.reserved;
        };
        Buffer(Buffer &&other)
            : len{other.len}, reserved{other.reserved}, data{std::move(other.data)},
              malloc{std::move(other.malloc)}
        {
            other.data = nullptr;
        }
        Buffer &operator=(Buffer &&other)
        {
            data = other.data;
            other.data = nullptr;
            reserved = other.reserved;
            len = other.len;
            malloc = std::move(other.malloc);
            return *this;
        }
        std::shared_ptr<T[]> array() const
        {
            return data;
        }
        T *array_ptr() const
        {
            return data.get();
        }
        T *clone_ptr() const
        {
            auto ptr = new T[len];
            std::memcpy(ptr, data.get(), sizeof(T) * len);
            return ptr;
        }

        size_t capacity() const
        {
            return reserved;
        }
        int32_t reserve(size_t size)
        {
            auto res = reserve_noexcept(size);
            if (res)
            {
                throw error::BaseException(common::ErrorCodes::MemoryError, "memory allocation failed");
            }
            return res;
        }

        int32_t reserve_noexcept(size_t size) noexcept
        {
            if (size > reserved)
            {
                data = std::move(malloc(size));
                if (!data)
                {

                    return EXIT_FAILURE;
                }
                reserved = size;
            }
            return EXIT_SUCCESS;
        }

        size_t size() const
        {
            return len;
        }
        // just set size
        void resize(size_t size)
        {
            len = size;
        }
        virtual ~Buffer()
        {
        }

    private:
        std::shared_ptr<T[]> data;
        size_t reserved;
        size_t len;

    private:
        Allocator malloc;
    };

};

#endif