#ifndef PIG_CORE_CORE_H
#define PIG_CORE_CORE_H

#include <cstdint>

namespace Pig {
    namespace Core {

        using page_id_t   = uint16_t;
        using page_size_t = uint16_t;

        constexpr uint8_t     PAGE_SIZE_KB = 4;
        constexpr page_size_t PAGE_SIZE_B  = PAGE_SIZE_KB * 1024;
        constexpr uint16_t    MAX_PAGES = 32768; // 4KB page enough for spacemap
        constexpr uint16_t    MAX_TABLES = 100;
    } // namespace Core
} // namespace Pig

#endif