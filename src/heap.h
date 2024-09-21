#ifndef PIGDB_CORE_HEAP_H
#define PIGDB_CORE_HEAP_H

#include <bitset>
#include <memory>
#include <sys/uio.h>

#include "error.h"
#include "util.h"

namespace Pig {
    namespace Core {

#define PAGE_SIZE_KB 4
#define MAX_PAGES 32768

        using PageId       = size_t;
        using PageSlot     = uint16_t;
        using TupleId      = std::pair<PageId, PageSlot>;
        using PageSizeType = uint16_t;

        class HeapFile {
          private:
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
                const PageId k_pageId;
                PageSlot     m_numSlots;
                PageSizeType m_freeBytes;
                // Fat pointer storing tupleoffset and length
                uint32_t *m_slotArray;
            };

            // Make sure fields are aligned.
            struct Tuple {
                uint16_t m_checksum;
                uint8_t *m_payload;
            };

          public:
            /**
             *
             * Create a uniquely owned HeapFile.
             * Note that current it does not grow.
             */
            [[nodiscard]] static std::unique_ptr<HeapFile> create();

            std::shared_ptr<Page> getPage(PageId pageId);

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