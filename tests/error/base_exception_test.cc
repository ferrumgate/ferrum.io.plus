#include <gtest/gtest.h>
#include "../../../../src/error/base_exception.h"

using namespace ferrum::io::error;
using namespace ferrum::io::common;
TEST(BaseExceptionTest, get_message_what)
{
    BaseException ex(ErrorCodes::RuntimeError, "hello exception");
    EXPECT_STREQ(ex.get_message().c_str(), "hello exception");
    EXPECT_STREQ(ex.what(), "hello exception");
}

TEST(BaseExceptionTest, copyConstructor)
{
    BaseException ex(ErrorCodes::RuntimeError, "hello exception");
    auto s = ex;
    EXPECT_STREQ(s.get_message().c_str(), "hello exception");
    EXPECT_STREQ(s.what(), "hello exception");
    EXPECT_EQ(s.get_error_code(), ErrorCodes::RuntimeError);
}