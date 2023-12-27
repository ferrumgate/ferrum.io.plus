#include "common.h"
namespace ferrum::io::common {

  Malloc *FuncTable::malloc{std::malloc};
  Realloc *FuncTable::realloc{std::realloc};
  UVDefaultLoop *FuncTable::uv_default_loop{::uv_default_loop};
  UVTcpInit *FuncTable::uv_tcp_init{::uv_tcp_init};
  UVReadStart *FuncTable::uv_read_start{::uv_read_start};
  UVTcpConnect *FuncTable::uv_tcp_connect{::uv_tcp_connect};
  UVWrite *FuncTable::uv_write{::uv_write};
  UVTcpBind *FuncTable::uv_tcp_bind{::uv_tcp_bind};
  UVFileNo *FuncTable::uv_fileno{::uv_fileno};
  UVListen *FuncTable::uv_listen{::uv_listen};

  UVUdpInit *FuncTable::uv_udp_init{::uv_udp_init};
  UVUdpInitEx *FuncTable::uv_udp_init_ex{::uv_udp_init_ex};
  UVUdpBind *FuncTable::uv_udp_bind{::uv_udp_bind};
  UVUdpReadStart *FuncTable::uv_udp_read_start{::uv_udp_recv_start};
  UVUdpWrite *FuncTable::uv_udp_write{::uv_udp_send};

  void FuncTable::reset() {
    FuncTable::malloc = std::malloc;
    FuncTable::realloc = std::realloc;
    FuncTable::uv_default_loop = ::uv_default_loop;
    // lib uv tcp
    FuncTable::uv_tcp_init = ::uv_tcp_init;
    FuncTable::uv_read_start = ::uv_read_start;
    FuncTable::uv_tcp_connect = ::uv_tcp_connect;
    FuncTable::uv_write = ::uv_write;
    FuncTable::uv_tcp_bind = ::uv_tcp_bind;
    FuncTable::uv_fileno = ::uv_fileno;
    FuncTable::uv_listen = ::uv_listen;
    // lib uv udp
    FuncTable::uv_udp_init = ::uv_udp_init;
    FuncTable::uv_udp_init_ex = ::uv_udp_init_ex;
    FuncTable::uv_udp_bind = ::uv_udp_bind;
    FuncTable::uv_udp_read_start = ::uv_udp_recv_start;
    FuncTable::uv_udp_write = ::uv_udp_send;
  }

}  // namespace ferrum::io::common