#ifndef PIGDB_CORE_UTIL_H
#define PIGDB_CORE_UTIL_H

#include "error.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <signal.h>
#include <sys/uio.h>

namespace Pig {
    namespace Core {

        enum class CompressionType : uint8_t { NONE = 0 };

#define PIG_ASSERT(cond, msg)                                                  \
    if (!(cond)) {                                                             \
        std::cerr << "Assertion failed: " << msg << std::endl;                 \
        std::cerr << "In file: " << __FILE__ << std::endl;                     \
        std::cerr << "At line: " << __LINE__ << std::endl;                     \
        std::cerr << "In function: " << __func__ << std::endl;                 \
        abort();                                                               \
    }

        class OwningIovec {
          public:
            OwningIovec()
                : m_base{std::unique_ptr<unsigned char[]>()}, m_len{0} {}

            OwningIovec(size_t size)
                : m_base{std::make_unique<unsigned char[]>(size)}, m_len{size} {
                memset(m_base.get(), 0, m_len);
            }

            // The readbuffer is owned by client.
            Error read(uint64_t offset, iovec outBuffer) {
                PIG_ASSERT(outBuffer.iov_base != nullptr,
                           "Read buffer sent to OwningIovec is null");
                PIG_ASSERT(offset + outBuffer.iov_len < m_len,
                           "Attempt to read beyond OwningVec buffer");
                memcpy(outBuffer.iov_base, m_base.get() + offset,
                       outBuffer.iov_len);
                return EMPRY_ERR;
            }

            Error write(uint64_t offset, iovec writeBuffer) {
                PIG_ASSERT(writeBuffer.iov_base != nullptr,
                           "Write buffer sent to OwningIovec is null");
                PIG_ASSERT(offset + writeBuffer.iov_len < m_len,
                           "Attempt to read beyond OwningVec buffer");
                memcpy(m_base.get() + offset, writeBuffer.iov_base,
                       writeBuffer.iov_len);
                return EMPRY_ERR;
            }

          private:
            std::unique_ptr<unsigned char[]> m_base;
            size_t                           m_len;
        };

    } // namespace Core
} // namespace Pig

#endif