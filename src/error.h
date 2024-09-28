#ifndef PIGDB_CORE_ERROR_H
#define PIGDB_CORE_ERROR_H

#include <exception>
#include <sstream>
#include <string>

namespace Pig {
    namespace Core {

// Quick error creation.
#define MKERROR(errCode, msg) Error((errCode), (msg))

// More verbose (and slightly slower)
#define MKERRORSITE(errCode, msg)                                              \
    Error((errCode), (msg), __FILE__, __LINE__, __func__)

#define EMPRY_ERR Error();

        using ErrCode = int16_t;

        struct Error : private std::exception {
            Error() : m_code{0} {}

            Error(ErrCode code, const std::string &message)
                : m_code{code}, m_message{message} {}

            Error(ErrCode code, const std::string &message, const char *file,
                  int line, const char *func)
                : m_code{code} {
                std::ostringstream oss;
                oss << "Error: " << message << " | Code: " << code
                    << " | Location: " << file << ":" << line << " in " << func;
                m_message = oss.str();
            }

            ErrCode code() const noexcept { return m_code; }

            explicit operator bool() const noexcept { return m_code != 0; }

            const char *what() const noexcept override {
                return m_message.c_str();
            }

          private:
            const ErrCode m_code;
            std::string   m_message;
        };

    } // namespace Core
} // namespace Pig

#endif // PIGDB_CORE_ERROR_H
