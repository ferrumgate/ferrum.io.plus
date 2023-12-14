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
        MemoryError,
    };

    /**
     * @brief libc functions for testing
     *
     */
    using Malloc = std::function<void *(size_t)>;
    using Realloc = std::function<void *(void *, size_t)>;

};

#endif