#ifndef PIGDB_CORE_HEAP_H
#define PIGDB_CORE_HEAP_H

#include <bitset>
#include <cstdint>
#include <cstring>
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <new>
#include <queue>
#include <shared_mutex>
#include <sys/uio.h>

#include "buffer_pool.h"
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
            struct Tuple {
                uint32_t m_checksum;
                iovec    m_payload;

                Tuple(uint32_t checksum, iovec payload)
                    : m_checksum{checksum}, m_payload{payload} {}
            };

            // Make sure fields are aligned.
            class Page {

              public:
                static constexpr page_size_t FREE_BYTES =
                    PAGE_SIZE_KB * 1024 - sizeof(page_id_t) - sizeof(PageSlot) -
                    sizeof(page_size_t);

                // TODO: check if this should be used
                explicit Page(page_id_t pageId)
                    : k_pageId{pageId}, m_numSlots{0} {
                    m_buffer.iov_base = nullptr;
                    m_buffer.iov_len  = 0;
                }

                Page(page_id_t pageId, iovec pageBuf) : k_pageId{pageId} {
                    PIG_ASSERT(pageBuf.iov_len == PAGE_SIZE_B,
                               "Page buffer not equals PAGE_SIZE_B");
                    auto base =
                        reinterpret_cast<unsigned char *>(pageBuf.iov_base);
                    auto page_id = reinterpret_cast<page_id_t *>(base);
                    PIG_ASSERT(
                        *page_id == k_pageId,
                        fmt::format("Page id in page buffer does not match"));

                    base += sizeof(k_pageId);

                    auto numSlots = reinterpret_cast<PageSlot *>(base);
                    m_numSlots    = *numSlots;

                    base += sizeof(m_numSlots);

                    auto freeBytes = reinterpret_cast<page_size_t *>(base);
                    m_freeBytes    = *freeBytes;

                    base += sizeof(m_freeBytes);

                    m_buffer.iov_base = reinterpret_cast<unsigned char *>(base);
                    m_buffer.iov_len  = FREE_BYTES;
                }

                void initPage(iovec pageBuf) {
                    PIG_ASSERT(m_buffer.iov_base == nullptr,
                               "Page to be inited has already set buffer");

                    auto base =
                        reinterpret_cast<unsigned char *>(pageBuf.iov_base);
                    auto page_id = reinterpret_cast<page_id_t *>(base);
                    *page_id     = k_pageId;

                    base += sizeof(k_pageId);

                    auto numSlots = reinterpret_cast<PageSlot *>(base);
                    *numSlots     = 0;

                    base += sizeof(m_numSlots);

                    auto freeBytes = reinterpret_cast<page_size_t *>(base);
                    *freeBytes     = FREE_BYTES;

                    base += sizeof(m_freeBytes);

                    m_buffer.iov_base = reinterpret_cast<unsigned char *>(base);
                    m_buffer.iov_len  = FREE_BYTES;
                }

                static page_size_t spaceForTuple(const Tuple &tuple) {
                    return sizeof(tuple.m_checksum) + tuple.m_payload.iov_len +
                           sizeof(uint32_t) /*for slot*/;
                }

                PageSlot addTuple(const Tuple &t) {
                    PIG_ASSERT(
                        spaceForTuple(t) <= m_freeBytes,
                        fmt::format("Not enough space in page for tuple"));

                    page_size_t tupleLength =
                        sizeof(t.m_checksum) + t.m_payload.iov_len;
                    uint16_t tupleOffsetInPage = m_freeBytes - 1 - tupleLength;
                    unsigned char *tupleOffset =
                        static_cast<unsigned char *>(m_buffer.iov_base) +
                        tupleOffsetInPage;
                    uint32_t *pageSlotOffset =
                        reinterpret_cast<uint32_t *>(m_buffer.iov_base) +
                        m_numSlots;
                    *pageSlotOffset = static_cast<PageSlot>(tupleOffsetInPage)
                                          << 16 |
                                      tupleLength;
                    // Copy the checksum
                    memcpy(tupleOffset, &t.m_checksum, sizeof(t.m_checksum));
                    memcpy(tupleOffset + sizeof(t.m_checksum),
                           t.m_payload.iov_base, t.m_payload.iov_len);

                    m_freeBytes -= spaceForTuple(t);

                    return m_numSlots++;
                }

                page_id_t getPageId() const { return k_pageId; }

                PageSlot getNumSlots() const { return m_numSlots; }

                page_size_t getFreeBytes() const { return m_freeBytes; }

#ifdef UNIT_TEST
                // Methods available only during unit testing
                iovec getBuffer() const { return m_buffer; }
#endif

              private:
                /** PAGE HEADER OF LENGTH 2 + 2 + 2 = 6 bytes */
                const page_id_t k_pageId;
                PageSlot        m_numSlots;
                page_size_t     m_freeBytes = FREE_BYTES;

                // In m_freeBytes, the slots occupy 4 bytes(offset + length)
                // The tuples are stored in the end.
                iovec m_buffer;
            };

          private:
            IoId_t m_id;
            Header m_header;

            std::shared_ptr<DiskManager> m_diskManager;
            std::shared_ptr<BufferPool>  m_bufferPool;

            /*
                This would be calculated from disk in future by reading pages.
                Each entry stores size in higher 16 bits and page id in lower.
            */
            std::priority_queue<uint32_t, std::vector<uint32_t>,
                                std::less<uint32_t>>
                m_freeSpaceMap;

            std::shared_mutex m_freeSpaceLock;

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
            Error addTuple(iovec tuple, TupleId &assignedTupleId);
        };
    } // namespace Core
} // namespace Pig

#endif // PIGDB_CORE_HEAP_H