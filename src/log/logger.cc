#include "logger.h"
namespace ferrum::io::log {
  Logger::Level Logger::_level = Logger::Level::INFO;

  void Logger::set_level(Level level) noexcept { Logger::_level = level; }
  Logger::Level Logger::get_level() noexcept { return Logger::_level; }
  void print_message(std::string_view type, std::string_view msg) {
    auto now = std::time(nullptr);
    std::tm tm = *std::localtime(&now);
    std::cout << "[" << std::put_time(&tm, "%c %Z") << "]"
              << "[" << type << "] " << msg << std::endl;
  }

  void Logger::error(std::string_view msg) {
    if(Logger::_level >= Logger::ERROR) print_message("ERROR", msg);
  }
  void Logger::fatal(std::string_view msg) {
    if(Logger::_level >= Logger::FATAL) print_message("FATAL", msg);
  }
  void Logger::warn(std::string_view msg) {
    if(Logger::_level >= Logger::WARN) print_message("WARN", msg);
  }
  void Logger::info(std::string_view msg) {
    if(Logger::_level >= Logger::INFO) print_message("INFO", msg);
  }
  void Logger::debug(std::string_view msg) {
    if(Logger::_level >= Logger::DEBUG) print_message("DEBUG", msg);
  }
}  // namespace ferrum::io::log