#include "common.h"
namespace ferrum::io::common
{

    Malloc *FuncTable::malloc{std::malloc};
    Realloc *FuncTable::realloc{std::realloc};
    UVDefaultLoop *FuncTable::uv_default_loop{::uv_default_loop};
    UVTcpInit *FuncTable::uv_tcp_init{::uv_tcp_init};
    UVReadStart *FuncTable::uv_read_start{::uv_read_start};
    UVTcpConnect *FuncTable::uv_tcp_connect{::uv_tcp_connect};
    UVWrite *FuncTable::uv_write{::uv_write};
    UVTcpBind *FuncTable::uv_tcp_bind{::uv_tcp_bind};

    void FuncTable::reset()
    {
        FuncTable::malloc = std::malloc;
        FuncTable::realloc = std::realloc;
        FuncTable::uv_default_loop = ::uv_default_loop;
        FuncTable::uv_tcp_init = ::uv_tcp_init;
        FuncTable::uv_read_start = ::uv_read_start;
        FuncTable::uv_tcp_connect = ::uv_tcp_connect;
        FuncTable::uv_write = ::uv_write;
        FuncTable::uv_tcp_bind = ::uv_tcp_bind;
    }

}