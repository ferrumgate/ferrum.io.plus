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
    ASSERT_ANY_THROW(Buffer<int32_t>(10,
                                     [](size_t a)
                                     { return nullptr; }););
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

    auto k = Buffer<int32_t>(0, [](size_t len)
                             { return nullptr; });
    auto result = k.reserve_noexcept(20);
    ASSERT_TRUE(result != 0);
}