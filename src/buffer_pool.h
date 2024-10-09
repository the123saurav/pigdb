#ifndef PIG_CORE_BUFFER_POOL_H
#define PIG_CORE_BUFFER_POOL_H

#include "core.h"
#include "disk-manager.h"
#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <array>
#include <atomic>
#include <cstddef>
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
        class BufferPool {
          public:
            BufferPool(size_t                       numFrames,
                       std::shared_ptr<DiskManager> diskManager);

          private:
            struct Frame {
                std::atomic_uint16_t m_pinCount;
                std::atomic_bool     m_dirty;
                // page in the buffer pool, not interpreted by buffer pool.
                unsigned char m_page[PAGE_SIZE_B];
            };

            void Evict();

            std::shared_ptr<DiskManager> m_diskManager;

            std::shared_mutex                              m_mutex;
            std::unordered_map<BufferPoolKey_t, FrameId_t> m_map;
            const std::vector<Frame>                       m_frames;
            lock_free_stack<FrameId_t>                     m_freeFrames;
        };
    } // namespace Core

} // namespace Pig

#endif