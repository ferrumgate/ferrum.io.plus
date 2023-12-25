#ifndef __COMMON_H__
#define __COMMON_H__

#include <uv.h>

#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <format>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace ferrum::io::common {

#define VERSION "1.0.0"
#define fill_zero(a) memset(&a, 0, sizeof(a))
#define fill_zero2(a, b) memset(a, 0, b)

  /**
   * @brief supported protocols
   */
  enum Protocol { Raw, Http, Http2, Http3, SSH, Rdp };

  /**
   * @brief errorcodes using with exceptions
   *
   */
  enum ErrorCodes {
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

  using UVDefaultLoop = uv_loop_t *(void);
  using UVTcpInit = int(uv_loop_t *, uv_tcp_t *);
  using UVReadStart = int(uv_stream_t *, uv_alloc_cb, uv_read_cb);
  using UVTcpConnect = int(uv_connect_t *, uv_tcp_t *, const struct sockaddr *,
                           uv_connect_cb);
  using UVWrite = int(uv_write_t *, uv_stream_t *, const uv_buf_t[],
                      unsigned int, uv_write_cb);
  using UVTcpBind = int(uv_tcp_t *, const struct sockaddr *, unsigned int);
  using UVFileNo = int(const uv_handle_t *, uv_os_fd_t *);
  using UVListen = int(uv_stream_t *, int, uv_connection_cb);

  /**
   * @brief For writing test codes for c functions
   I think best easy way, no performance overhead
   *
   */
  struct FuncTable {
    static Malloc *malloc;
    static Realloc *realloc;
    static UVDefaultLoop *uv_default_loop;
    static UVTcpInit *uv_tcp_init;
    static UVReadStart *uv_read_start;
    static UVTcpConnect *uv_tcp_connect;
    static UVWrite *uv_write;
    static UVTcpBind *uv_tcp_bind;
    static UVFileNo *uv_fileno;
    static UVListen *uv_listen;
    static void reset();
  };

};  // namespace ferrum::io::common

#endif