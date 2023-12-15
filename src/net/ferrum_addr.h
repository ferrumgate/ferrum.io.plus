#ifndef __FERRUM_ADDR_H__
#define __FERRUM_ADDR_H__

#include "../common/common.h"
#include "../error/base_exception.h"
#include <uv.h>

namespace ferrum::io::net
{

    class FerrumAddr
    {

    public:
        FerrumAddr(const std::string &ip, uint16_t port = 0);
        FerrumAddr(const FerrumAddr &addr);
        FerrumAddr &operator=(const FerrumAddr &addr);
        FerrumAddr(FerrumAddr &&addr) = delete;
        FerrumAddr &operator=(FerrumAddr &&addr) = delete;
        virtual ~FerrumAddr() = default;
        bool is_ipv4() const;
        bool is_ipv6() const;
        std::string to_string(bool print_port = false) const;
        const sockaddr_in *get_addr4() const;
        const sockaddr_in6 *get_addr6() const;
        const sockaddr *get_addr() const;

    private:
        std::variant<sockaddr_in, sockaddr_in6> addr;
        uint16_t port;
        std::string ip;
        bool is_ipv4_address;
    };
}
#endif