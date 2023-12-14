#include <gtest/gtest.h>
#include "../../../../src/log/logger.h"

using namespace ferrum::io::log;

TEST(LoggerTest, set_level_get_level)
{
    Logger::set_level(Logger::Level::FATAL);
    ASSERT_EQ(Logger::Level::FATAL, Logger::get_level());
}
TEST(LoggerTest, writeLog)
{
    Logger::set_level(Logger::Level::DEBUG);
    Logger::error("hello error");
}