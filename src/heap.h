#ifndef PIGDB_CORE_HEAP_H
#define PIGDB_CORE_HEAP_H

#include <bitset>
#include <iterator>
#include <memory>
#include <new>
#include <sys/uio.h>

#include "core.h"
#include "disk-manager.h"
#include "error.h"
#include "util.h"

namespace Pig {
    namespace Core {

        using PageSlot = uint16_t;
        using TupleId  = std::pair<page_id_t, PageSlot>;

        class HeapFile {
          public:
            // Make sure fields are aligned.
            struct Header {
                // Compression type for pages, note that header is uncompressed.
                CompressionType m_compression = CompressionType::NONE;
            };

            // Make sure fields are aligned.
            struct Page {
                static constexpr page_size_t FREE_BYTES =
                    PAGE_SIZE_KB * 1024 - sizeof(page_id_t) - sizeof(PageSlot) -
                    sizeof(page_size_t);

                const page_id_t k_pageId;
                PageSlot        m_numSlots;
                page_size_t     m_freeBytes = FREE_BYTES;
                // Fat pointer storing tupleoffset and length
                unsigned char m_buffer[FREE_BYTES];

                explicit Page(page_id_t pageId)
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
            IoId_t m_id;
            Header m_header;
            // Below fields are stored in separate pages
            std::bitset<MAX_PAGES> m_lessThan33UsedPages;
            std::bitset<MAX_PAGES> m_lessThan66UsedPages;
            std::bitset<MAX_PAGES> m_lessThan100UsedPages;

            Page                        *m_pages;
            std::shared_ptr<DiskManager> m_diskManager;

            HeapFile(std::shared_ptr<DiskManager> diskManager);

          public:
            HeapFile(const HeapFile &) = delete;
            HeapFile(HeapFile &&)      = delete;

            HeapFile *operator=(const HeapFile &) = delete;
            HeapFile *operator=(HeapFile &&)      = delete;

            /**
             *
             * Create a uniquely owned HeapFile.
             * Note that current it does not grow.
             */
            [[nodiscard]] static std::unique_ptr<HeapFile>
            create(std::shared_ptr<DiskManager> diskManager);

            Page *getPage(page_id_t pageId) const noexcept;

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