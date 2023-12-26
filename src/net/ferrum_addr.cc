#include "ferrum_addr.h"
namespace ferrum::io::net {
  FerrumAddr::FerrumAddr(const std::string &ip, uint16_t port)
      : ip_{ip}, port_{port}, is_ipv4_address_{true}, is_parsed_{true} {
    sockaddr_in6 sock6;
    if(uv_ip6_addr(ip.c_str(), port, &sock6) < 0) {
      sockaddr_in sock4;
      if(uv_ip4_addr(ip.c_str(), port, &sock4) < 0) {
        throw ferrum::io::error::BaseException(common::ErrorCodes::ConvertError,
                                               "ip could not parsed");
      } else
        addr_ = sock4;
    } else {
      is_ipv4_address_ = false;
      addr_ = sock6;
    }
  }
  FerrumAddr::FerrumAddr(const sockaddr *saddr, bool is_parsed)
      : ip_(), port_{0}, is_ipv4_address_{true}, is_parsed_{is_parsed_} {
    if(saddr->sa_family == AF_INET) {
      is_ipv4_address_ = true;

      auto addr4 = reinterpret_cast<const sockaddr_in *>(saddr);
      if(is_parsed_) {
        char dest[128] = {0};
        uv_ip4_name(addr4, dest, 128);
        port_ = ntohs(addr4->sin_port);
        ip_ = dest;
      }
      addr_ = *addr4;

      return;
    } else if(saddr->sa_family == AF_INET6) {
      is_ipv4_address_ = false;

      auto addr6 = reinterpret_cast<const sockaddr_in6 *>(saddr);
      if(is_parsed_) {
        char dest[128] = {0};
        uv_ip6_name(addr6, dest, 128);
        port_ = ntohs(addr6->sin6_port);
        ip_ = dest;
      }
      addr_ = *addr6;

      return;
    } else
      throw ferrum::io::error::BaseException(common::ErrorCodes::ConvertError,
                                             "cannot init to FerrumAddr");
  }

  FerrumAddr::FerrumAddr(const FerrumAddr &other) noexcept
      : addr_{other.addr_},
        ip_{other.ip_},
        port_{other.port_},
        is_ipv4_address_{other.is_ipv4_address_},
        is_parsed_{other.is_parsed_} {}

  FerrumAddr &FerrumAddr::operator=(const FerrumAddr &other) noexcept {
    addr_ = other.addr_;
    ip_ = other.ip_;
    port_ = other.port_;
    is_ipv4_address_ = other.is_ipv4_address_;
    is_parsed_ = other.is_parsed_;
    return *this;
  }

  bool FerrumAddr::is_ipv4() const noexcept { return is_ipv4_address_; }
  bool FerrumAddr::is_ipv6() const noexcept { return !is_ipv4_address_; }
  std::string FerrumAddr::to_string(bool print_port) noexcept {
    if(is_ipv4_address_) {
      if(!is_parsed_) {
        is_parsed_ = true;
        char dest[128] = {0};
        auto addr4 = get_addr4();
        uv_ip4_name(addr4, dest, 128);
        port_ = ntohs(addr4->sin_port);
        ip_ = dest;
      }
      if(print_port) return std::format("{}:{}", ip_, port_);

      return std::format("{}", ip_);
    } else {
      if(!is_parsed_) {  // if not parsed for performance reasons parse it
        is_parsed_ = true;
        char dest[128] = {0};
        auto addr6 = get_addr6();
        uv_ip6_name(addr6, dest, 128);
        port_ = ntohs(addr6->sin6_port);
        ip_ = dest;
      }

      if(print_port) return std::format("[{}]:{}", ip_, port_);

      return std::format("{}", ip_);
    }
  }

  const sockaddr_in *FerrumAddr::get_addr4() const noexcept {
    return std::get_if<sockaddr_in>(&this->addr_);
  }
  const sockaddr_in6 *FerrumAddr::get_addr6() const noexcept {
    return std::get_if<sockaddr_in6>(&this->addr_);
  }
  const sockaddr *FerrumAddr::get_addr() const noexcept {
    if(is_ipv4_address_) {
      return reinterpret_cast<const sockaddr *>(
          std::get_if<sockaddr_in>(&this->addr_));
    }
    return reinterpret_cast<const sockaddr *>(
        std::get_if<sockaddr_in6>(&this->addr_));
  }

}  // namespace ferrum::io::net