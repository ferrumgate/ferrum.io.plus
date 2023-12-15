#ifndef __COMMON_H__
#define __COMMON_H__

#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <list>
#include <array>
#include <map>
#include <set>
#include <memory>
#include <cstdint>
#include <exception>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <functional>
#include <variant>
#include <format>
#include <cstring>

namespace ferrum::io::common
{

#define VERSION "1.0.0"
#define fill_zero(a) memset(&a, 0, sizeof(a))
#define fill_zero2(a, b) memset(a, 0, b)

    /**
     * @brief supported protocols
     */
    enum Protocol
    {
        Raw,
        Http,
        Http2,
        Http3,
        SSH,
        RDP
    };

    /**
     * @brief errorcodes using with exceptions
     *
     */
    enum ErrorCodes
    {
        RuntimeError,
        AppError,
        ConvertError,
        SocketError,
        SocketClosedError,
        MemoryError,
    };

    /**
     * @brief libc functions for testing
     *
     */
    using Malloc = void *(size_t);
    using Realloc = void *(void *, size_t);

    using UV_tcp_init = int(uv_loop_t *, uv_tcp_t *);

};

#endif