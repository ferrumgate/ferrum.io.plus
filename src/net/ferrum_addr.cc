#include "ferrum_addr.h"
namespace ferrum::io::net
{
    FerrumAddr::FerrumAddr(const std::string &ip, uint16_t port)
        : ip{ip}, port{port}, is_ipv4_address{true}
    {

        sockaddr_in6 sock6;
        if (uv_ip6_addr(ip.c_str(), port, &sock6) < 0)
        {
            sockaddr_in sock4;
            if (uv_ip4_addr(ip.c_str(), port, &sock4) < 0)
            {
                throw ferrum::io::error::BaseException(common::ErrorCodes::ConvertError, "ip could not parsed");
            }
            else
                addr = sock4;
        }
        else
        {
            is_ipv4_address = false;
            addr = sock6;
        }
    }
    FerrumAddr::FerrumAddr(const sockaddr *saddr) : ip(), port{0}, is_ipv4_address{true}
    {
        if (saddr->sa_family == AF_INET)
        {
            is_ipv4_address = true;
            char dest[128] = {0};
            auto addr4 = reinterpret_cast<const sockaddr_in *>(saddr);
            uv_ip4_name(addr4, dest, 128);
            port = ntohs(addr4->sin_port);
            addr = *addr4;
            ip = dest;
            return;
        }
        else if (saddr->sa_family == AF_INET6)
        {
            is_ipv4_address = false;
            char dest[128] = {0};
            auto addr6 = reinterpret_cast<const sockaddr_in6 *>(saddr);
            uv_ip6_name(addr6, dest, 128);
            port = ntohs(addr6->sin6_port);
            addr = *addr6;
            ip = dest;
            return;
        }
        else
            throw ferrum::io::error::BaseException(common::ErrorCodes::ConvertError, "cannot init to FerrumAddr");
    }

    FerrumAddr::FerrumAddr(const FerrumAddr &other) noexcept
        : addr{other.addr}, ip{other.ip}, port{other.port}, is_ipv4_address{other.is_ipv4_address}
    {
    }

    FerrumAddr &FerrumAddr::operator=(const FerrumAddr &other) noexcept
    {
        addr = other.addr;
        ip = other.ip;
        port = other.port;
        is_ipv4_address = other.is_ipv4_address;
        return *this;
    }

    bool FerrumAddr::is_ipv4() const noexcept
    {
        return is_ipv4_address;
    }
    bool FerrumAddr::is_ipv6() const noexcept
    {
        return !is_ipv4_address;
    }
    std::string FerrumAddr::to_string(bool print_port) const noexcept
    {
        if (is_ipv4_address)
        {
            if (print_port)
                return std::format("{}:{}", ip, port);

            return std::format("{}", ip);
        }
        else
        {
            if (print_port)
                return std::format("[{}]:{}", ip, port);

            return std::format("{}", ip);
        }
    }

    const sockaddr_in *FerrumAddr::get_addr4() const noexcept
    {
        return std::get_if<sockaddr_in>(&this->addr);
    }
    const sockaddr_in6 *FerrumAddr::get_addr6() const noexcept
    {
        return std::get_if<sockaddr_in6>(&this->addr);
    }
    const sockaddr *FerrumAddr::get_addr() const noexcept
    {
        if (is_ipv4_address)
        {
            return reinterpret_cast<const sockaddr *>(std::get_if<sockaddr_in>(&this->addr));
        }
        return reinterpret_cast<const sockaddr *>(std::get_if<sockaddr_in6>(&this->addr));
    }

}