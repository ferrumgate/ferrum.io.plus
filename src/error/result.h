#ifndef __RESULT_H__
#define __RESULT_H__

#include "../common/common.h"
namespace ferrum::io::error {
  template <typename T, typename Err>
  class Result {
   public:
    Result(T &t) : value{t}, is_error{false} {}
    Result(Err &e) : value{e}, is_error{true} {}
    Result(Result &other) : value{other.value}, is_error{other.is_error} {}
    constexpr bool is_err() const { return this->is_err; }
    constexpr bool is_ok() const { return !this->is_err; }
    constexpr Err &get_err() const { return std::get<Err>(this->value); }
    constexpr T &get_value() const { return std::get<T>(this->value); }

   private:
    std::variant<T, Err> value;
    bool is_error;
  };

}  // namespace ferrum::io::error

#endif