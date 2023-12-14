#include "base_exception.h"

namespace ferrum::io::error
{

    BaseException::BaseException(common::ErrorCodes code, const std::string &msg)
        : std::exception(), message(msg), error_code(code)
    {
    }
    BaseException::BaseException(const BaseException &ex)
        : std::exception(ex), message(ex.message), error_code(ex.error_code)
    {
    }
    BaseException::BaseException(const BaseException &&ex)
        : std::exception(ex), message(ex.message), error_code(ex.error_code)
    {
    }
    BaseException &BaseException::operator=(const BaseException &ex)
    {
        BaseException::exception::operator=(ex);
        this->message = ex.message;
        this->error_code = ex.error_code;
        return *this;
    }
    BaseException &BaseException::operator=(const BaseException &&ex)
    {
        BaseException::exception::operator=(ex);
        this->message = ex.message;
        this->error_code = ex.error_code;
        return *this;
    }

    std::string BaseException::get_message() const noexcept
    {
        return this->message;
    }
    common::ErrorCodes BaseException::get_error_code() const noexcept
    {
        return this->error_code;
    }

    const char *
    BaseException::what() const noexcept
    {
        return this->message.c_str();
    }

}