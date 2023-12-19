#include <gtest/gtest.h>
#include "../../../../src/memory/buffer.h"

using namespace ferrum::io::memory;

TEST(BufferTest, constructor)
{
    auto k = Buffer<int32_t>(5);
    ASSERT_EQ(k.size(), 5);
    ASSERT_EQ(k.capacity(), 5);
    ASSERT_TRUE(k.array() != nullptr);
}

TEST(BufferTest, constructor_zero)
{
    auto k = Buffer<int32_t>();
    ASSERT_EQ(k.size(), 0);
    ASSERT_EQ(k.capacity(), 0);
    ASSERT_TRUE(k.array() == nullptr);
}

TEST(BufferTest, constructor_exception)
{
    auto func = [](size_t a)
    { return std::shared_ptr<int32_t[]>{}; };
    ASSERT_ANY_THROW(Buffer<int32_t>(10, func));
}

TEST(BufferTest, move_constructor)
{
    auto k = Buffer<int32_t>(5);
    ASSERT_EQ(k.size(), 5);
    ASSERT_EQ(k.capacity(), 5);
    ASSERT_TRUE(k.array() != nullptr);

    auto ptr = k.array();
    auto k1{std::move(k)};
    ASSERT_EQ(k1.size(), 5);
    ASSERT_EQ(k1.capacity(), 5);
    ASSERT_TRUE(k1.array() != nullptr);
    ASSERT_TRUE(ptr == k1.array());
    ASSERT_TRUE(k.array() == nullptr);
}
TEST(BufferTest, move_operator)
{
    auto k = Buffer<int32_t>(5);
    ASSERT_EQ(k.size(), 5);
    ASSERT_EQ(k.capacity(), 5);
    ASSERT_TRUE(k.array() != nullptr);

    auto ptr = k.array();
    auto k1 = std::move(k);
    ASSERT_EQ(k1.size(), 5);
    ASSERT_EQ(k1.capacity(), 5);
    ASSERT_TRUE(k1.array() != nullptr);
    ASSERT_TRUE(ptr == k1.array());
    ASSERT_TRUE(k.array() == nullptr);
}

TEST(BufferTest, copy_constructor)
{
    auto k = Buffer<int32_t>(5);
    ASSERT_EQ(k.size(), 5);
    ASSERT_EQ(k.capacity(), 5);
    ASSERT_TRUE(k.array() != nullptr);

    auto ptr = k.array();
    auto k1{k};
    ASSERT_EQ(k1.size(), 5);
    ASSERT_EQ(k1.capacity(), 5);
    ASSERT_TRUE(k1.array() != nullptr);
    ASSERT_TRUE(ptr != k1.array());
    ASSERT_TRUE(k.array() != nullptr);
}
TEST(BufferTest, copy_operator)
{
    auto k = Buffer<int32_t>(5);
    ASSERT_EQ(k.size(), 5);
    ASSERT_EQ(k.capacity(), 5);
    ASSERT_TRUE(k.array() != nullptr);

    auto ptr = k.array();
    auto k1 = k;
    ASSERT_EQ(k1.size(), 5);
    ASSERT_EQ(k1.capacity(), 5);
    ASSERT_TRUE(k1.array() != nullptr);
    ASSERT_TRUE(ptr != k1.array());
    ASSERT_TRUE(k.array() != nullptr);
}

TEST(BufferTest, reserve)
{
    auto k = Buffer<int32_t>();
    ASSERT_EQ(k.size(), 0);
    ASSERT_EQ(k.capacity(), 0);
    ASSERT_TRUE(k.array() == nullptr);

    k.reserve(10);
    ASSERT_EQ(k.size(), 0);
    ASSERT_EQ(k.capacity(), 10);
    ASSERT_TRUE(k.array() != nullptr);
    (*k.array_ptr()) = 10;
    k.resize(1);
    ASSERT_EQ(k.size(), 1);
}

TEST(BufferTest, reserve_with_error)
{

    auto func = [](size_t a)
    { return std::shared_ptr<int32_t[]>{}; };
    auto k = Buffer<int32_t>(size_t(0), func);

    auto result = k.reserve_noexcept(20);
    ASSERT_TRUE(result != 0);
}

TEST(BufferTest, to_string)
{
    char data[] = {'h', 'e', 'l', 'l', 'o', 0};
    auto k = Buffer<char>(6);
    std::copy(data, data + 6, k.array_ptr());
    auto str = k.to_string();
    ASSERT_STREQ(str.c_str(), "hello");
    // auto px =
}

TEST(BufferTest, to_string2)
{
    char data[] = {'h', 'e', 'l', 'l', 'o', 0};

    auto k = Buffer<char>(data, 6);
    auto str = k.to_string();
    ASSERT_STREQ(str.c_str(), "hello");
    // auto px =
}