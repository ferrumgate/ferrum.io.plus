#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "../common/common.h"

namespace ferrum::io::log {

  class Logger {
   public:
    enum Level { FATAL, ERROR, WARN, INFO, DEBUG };

    static void set_level(Level level) noexcept;
    static Level get_level() noexcept;
    static void error(std::string_view msg);
    static void fatal(std::string_view msg);
    static void warn(std::string_view msg);
    static void info(std::string_view msg);
    static void debug(std::string_view msg);

   private:
    Logger() = default;
    Logger(const Logger &logger) = default;
    ~Logger() = default;
    static Level _level;
  };

};  // namespace ferrum::io::log

#endif