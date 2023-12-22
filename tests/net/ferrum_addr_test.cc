#include <gtest/gtest.h>
#include "../../../../src/net/ferrum_addr.h"

using namespace ferrum::io::net;
TEST(FerrumAddrTest, constructor)
{
    auto addr = FerrumAddr("127.0.0.1");
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_FALSE(addr.is_ipv6());
    EXPECT_STREQ(addr.to_string().c_str(), "127.0.0.1");
    EXPECT_STREQ(addr.to_string(true).c_str(), "127.0.0.1:0");
}

TEST(FerrumAddrTest, ipv4)
{
    auto addr = FerrumAddr("127.0.0.1");
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_FALSE(addr.is_ipv6());
    EXPECT_STREQ(addr.to_string().c_str(), "127.0.0.1");
    EXPECT_STREQ(addr.to_string(true).c_str(), "127.0.0.1:0");
}

TEST(FerrumAddrTest, ipv4_from_sockaddr)
{

    auto saddr = sockaddr_in{};
    uv_ip4_addr("127.127.100.1", 1984, &saddr);
    auto faddr = FerrumAddr{reinterpret_cast<const sockaddr *>(&saddr)};

    EXPECT_TRUE(faddr.is_ipv4());
    EXPECT_FALSE(faddr.is_ipv6());
    EXPECT_STREQ(faddr.to_string().c_str(), "127.127.100.1");
    EXPECT_STREQ(faddr.to_string(true).c_str(), "127.127.100.1:1984");
}

TEST(FerrumAddrTest, ipv4_zero)
{
    auto addr = FerrumAddr("0.0.0.0");
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_FALSE(addr.is_ipv6());
    EXPECT_STREQ(addr.to_string().c_str(), "0.0.0.0");
}

TEST(FerrumAddrTest, ipv6)
{
    auto addr = FerrumAddr("::", 1000);
    EXPECT_FALSE(addr.is_ipv4());
    EXPECT_TRUE(addr.is_ipv6());
    EXPECT_STREQ(addr.to_string().c_str(), "::");
    EXPECT_STREQ(addr.to_string(true).c_str(), "[::]:1000");
}

TEST(FerrumAddrTest, get_addr4)
{
    auto addr = FerrumAddr("127.0.0.1", 1000);
    auto addr4 = addr.get_addr4();
    EXPECT_EQ(addr4->sin_family, AF_INET);
    EXPECT_EQ(addr4->sin_port, ntohs(1000));
    EXPECT_EQ(addr4->sin_addr.s_addr >> 24, 1);
    EXPECT_EQ(addr4->sin_addr.s_addr << 16 >> 24, 0);
    EXPECT_EQ(addr4->sin_addr.s_addr << 24 >> 24, 127);
}

TEST(FerrumAddrTest, memory_address4_equal)
{
    auto addr = FerrumAddr("127.0.0.1", 1000);
    auto addr4 = addr.get_addr4();
    auto addrg = addr.get_addr();
    EXPECT_EQ(reinterpret_cast<const void *>(addr4), reinterpret_cast<const void *>(addrg));
    EXPECT_EQ(addrg->sa_family, AF_INET);
}

TEST(FerrumAddrTest, memory_address6_equal)
{
    auto addr = FerrumAddr("::", 1000);
    auto addr6 = addr.get_addr6();
    auto addrg = addr.get_addr();
    EXPECT_EQ(reinterpret_cast<const void *>(addr6), reinterpret_cast<const void *>(addrg));
}
