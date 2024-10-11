#ifndef PIG_CORE_BUFFER_POOL_H
#define PIG_CORE_BUFFER_POOL_H

#include "core.h"
#include "disk-manager.h"
#include "error.h"
#include "lock_free_stack.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
namespace Pig {

    namespace Core {

        // first 16 bits store io id and last 48 bits store page id within
        // io_id.
        using BufferPoolKey_t = uint64_t;
        using FrameId_t       = size_t;

        /**
        A buffer pool stores pages of Heap and Index file in fixed size frames.
        In current implementation there is only 1 buffer pool.

        The pool is pre-allocated and does not grow today.
        The buffer pool handles misses by fetching from disk manager.

        On get, it first sees if page is in buffer pool, if yes pins it and
        returns the page raw contents.
        If not, it needs to fetch from Disk.
        For this it gets a free frame for free list and pins it.
        If it can't it evicts a page, if dirty flushing it to disk
        synchronously.
         */
        class BufferPool : public std::enable_shared_from_this<BufferPool> {
          private:
            struct Frame {
                std::atomic_uint16_t m_pinCount;
                std::atomic_bool     m_dirty;
                // page in the buffer pool, not interpreted by buffer pool.
                unsigned char m_page[PAGE_SIZE_B];

                Frame() : m_pinCount{0}, m_dirty{false} {}
            };

            class BufferPoolPageGuard {

              public:
                BufferPoolPageGuard(Frame &f) : m_frame(f) {
                    m_frame.m_pinCount++;
                }

                BufferPoolPageGuard(const BufferPoolPageGuard &) = delete;
                BufferPoolPageGuard(BufferPoolPageGuard &&)      = delete;

                BufferPoolPageGuard &
                operator=(const BufferPoolPageGuard &)                 = delete;
                BufferPoolPageGuard &operator=(BufferPoolPageGuard &&) = delete;

                ~BufferPoolPageGuard() { m_frame.m_pinCount--; }

                void markDirty() { m_frame.m_dirty.store(true); }

              private:
                Frame &m_frame;
            };

          public:
            BufferPool(size_t                       numFrames,
                       std::shared_ptr<DiskManager> diskManager);

            /**
                Gets a page from bufferpool.
                If the page is in pool, its latest.
                If the page is not present, tries to reserve a frame from
                free list and then reads it from disk assuming the page is
                valid. If there are no frames, it evicts the first unpinned page
                from LRU.
             */
            BufferPoolPageGuard GetPage(IoId_t io_id, page_id_t page_id);

          private:
            void evictPage();

            [[nodiscard]] Error flushPage(IoId_t io_id, page_id_t page_id);

            [[nodiscard]] Error readPageFromDisk(IoId_t    io_id,
                                                 page_id_t page_id,
                                                 iovec     buffer) const;

            std::shared_ptr<DiskManager> m_diskManager;

            std::shared_mutex m_mutex;
            // FrameId_t is index in m_frames.
            std::unordered_map<BufferPoolKey_t, FrameId_t> m_map;
            std::vector<Frame>                             m_frames;
            LockFreeStack<FrameId_t>                       m_freeFrames;
        };
    } // namespace Core

} // namespace Pig

#endif