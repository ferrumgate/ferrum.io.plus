#ifndef __BASE_EXCEPTION_H__
#define __BASE_EXCEPTION_H__

#include "../common/common.h"

namespace ferrum::io::error
{

    class BaseException : public std::exception
    {
    public:
        BaseException(common::ErrorCodes code, const std::string &msg);
        BaseException(const BaseException &ex);
        BaseException(const BaseException &&ex);
        BaseException &operator=(const BaseException &ex);
        BaseException &operator=(const BaseException &&ex);
        ~BaseException() = default;
        virtual const char *what() const noexcept override;
        std::string get_message() const noexcept;
        common::ErrorCodes get_error_code() const noexcept;

    protected:
        std::string message;
        common::ErrorCodes error_code;
    };

};

#endif