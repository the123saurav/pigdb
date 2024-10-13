#include "heap.h"
#include "core.h"
#include "error.h"
#include "util.h"
#include <cstddef>
#include <cstdint>
#include <memory>

#include <fmt/core.h>
#include <fmt/format.h>
#include <shared_mutex>
#include <sys/uio.h>

namespace Pig {
    namespace Core {

        HeapFile::HeapFile(std::shared_ptr<DiskManager> diskManager)
            : m_diskManager{std::move(diskManager)} {
            // Use diskManager to intialize new file
            m_id = m_diskManager->registerFile(MAX_PAGES * PAGE_SIZE_KB +
                                               4 /* header + 3 spacemap */ *
                                                   PAGE_SIZE_KB);

            // CREATE PAGES in put to disk, TODO: initialize header
            size_t offset = PAGE_SIZE_B;
            for (uint32_t i = 0; i < MAX_PAGES; ++i) {
                auto page = Page(i);

                iovec buf;
                auto  buffer = std::make_unique<unsigned char[]>(PAGE_SIZE_B);

                buf.iov_base = buffer.get();
                buf.iov_len  = PAGE_SIZE_B;

                auto err = m_diskManager->read(m_id, offset, buf);
                PIG_ASSERT(!err, "Page read failed");
                page.initPage(buf);
                auto err2 = m_diskManager->write(m_id, offset, buf);
                PIG_ASSERT(!err2, "Page write failed");

                offset += PAGE_SIZE_B;

                m_freeSpaceMap.push(Page::FREE_BYTES << 16 | i);
            }
        }

        std::unique_ptr<HeapFile>
        HeapFile::create(std::shared_ptr<DiskManager> diskManager) {
            return std::unique_ptr<HeapFile>(
                new HeapFile(std::move(diskManager)));
        }

        HeapFile::Page *HeapFile::getPage(page_id_t pageId) const noexcept {
            PIG_ASSERT(pageId < MAX_PAGES,
                       fmt::format("Invalid PageId {} requested", pageId));
            // TODO delegate to buffer pool
            return nullptr;
        }

        Error HeapFile::addTuple(iovec tuple, TupleId &assignedTupleId) {
            auto checksum          = calculateChecksum(tuple);
            auto t                 = Tuple(checksum, tuple);
            auto spaceNeededInPage = Page::spaceForTuple(t);

            // Locate a page for it from space map.
            std::shared_lock lock(m_freeSpaceLock);
            const uint32_t  &top = m_freeSpaceMap.top();
            PIG_ASSERT((top >> 16) > spaceNeededInPage,
                       fmt::format("No space available in heap file for tuple "
                                   "of size {}, top space: {}",
                                   spaceNeededInPage, (top >> 16)));
            m_freeSpaceMap.pop();
            lock.unlock();
            // At this point, the page is not in freespacemap, so it can not
            // be updated concurrently for other INSERTs

            /*
            Now add tuple to the page:
            1. Get the page from buffer pool.
            2. Get the slotId for insert
            3. Generate checksum for tuple and add the payload to page buffer
            via memcpy based on m_freeBytes to locate the offset within page
            4. Update the m_freeBytes
            5. Mark page as dirty
            6. Add back to freeSpaceMap
            7. Return pageId, tupleId
            */
            PageSlot slot;
            auto     page_id = top & 0xFFFF;
            {
                auto pageGuard = m_bufferPool->GetPage(m_id, page_id);

                iovec pageBuf = pageGuard.getRawPage();

                auto heapPage = HeapFile::Page(page_id, pageBuf);

                slot = heapPage.addTuple(t);

                pageGuard.markDirty();
            }

            uint32_t newEntry =
                ((top >> 16) - spaceNeededInPage) << 16 | page_id;

            m_freeSpaceMap.push(newEntry);

            assignedTupleId.first  = page_id;
            assignedTupleId.second = slot;
            return EMPRY_ERR;
        }

    } // namespace Core
} // namespace Pig