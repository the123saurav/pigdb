#ifndef PIGDB_CORE_UTIL_H
#define PIGDB_CORE_UTIL_H

#include <cstdint>
#include <iostream>
#include <signal.h>

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

    } // namespace Core
} // namespace Pig

#endif