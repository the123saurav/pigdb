#ifndef PIGDB_CORE_HEAP_H
#define PIGDB_CORE_HEAP_H

#include <bitset>
#include <iterator>
#include <memory>
#include <new>
#include <sys/uio.h>

#include "error.h"
#include "util.h"

namespace Pig {
    namespace Core {

        constexpr uint8_t  PAGE_SIZE_KB = 4;
        constexpr uint16_t MAX_PAGES    = 32768;

        using PageId       = uint16_t;
        using PageSlot     = uint16_t;
        using TupleId      = std::pair<PageId, PageSlot>;
        using PageSizeType = uint16_t;

        class HeapFile {
          public:
            // Make sure fields are aligned.
            struct Header {
                // Compression type for pages, note that header is uncompressed.
                CompressionType        m_compression = CompressionType::NONE;
                std::bitset<MAX_PAGES> m_lessThan33UsedPages;
                std::bitset<MAX_PAGES> m_lessThan66UsedPages;
                std::bitset<MAX_PAGES> m_lessThan100UsedPages;
            };

            // Make sure fields are aligned.
            struct Page {
                static constexpr PageSizeType FREE_BYTES =
                    PAGE_SIZE_KB * 1024 - sizeof(PageId) - sizeof(PageSlot) -
                    sizeof(PageSizeType);

                const PageId k_pageId;
                PageSlot     m_numSlots;
                PageSizeType m_freeBytes = FREE_BYTES;
                // Fat pointer storing tupleoffset and length
                unsigned char m_buffer[FREE_BYTES];

                explicit Page(PageId pageId)
                    : k_pageId{pageId}, m_numSlots{0} {}
            };

            static_assert(
                sizeof(Page) == PAGE_SIZE_KB * 1024,
                "Page struct size does not match expected page size.");

            // Make sure fields are aligned.
            struct Tuple {
                uint16_t m_checksum;
                uint8_t *m_payload;
            };

          private:
            Header m_header;
            Page  *m_pages;

            HeapFile();

          public:
            HeapFile(const HeapFile &) = delete;
            HeapFile(HeapFile &&)      = delete;

            HeapFile *operator=(const HeapFile &) = delete;
            HeapFile *operator=(HeapFile &&)      = delete;

            ~HeapFile();

            /**
             *
             * Create a uniquely owned HeapFile.
             * Note that current it does not grow.
             */
            [[nodiscard]] static std::unique_ptr<HeapFile> create();

            Page *getPage(PageId pageId) const noexcept;

            /**
             * Assigns a tuple to a page slot in heap.
             * The heap file owns the logic to a page based on free space
             * map and returns the pageId and slot.
             * The space is reserved in page and the buffer pool
             *
             * In future, if there are no page,it would trigger growth.
             */
            Error addTuple(iovec *tuples, TupleId &assignedTupleId);
        };
    } // namespace Core
} // namespace Pig

#endif // PIGDB_CORE_HEAP_H