#include "buffer_pool.h"
#include "core.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <sys/uio.h>

namespace Pig {

    namespace Core {

        BufferPool::BufferPool(size_t                       numFrames,
                               std::shared_ptr<DiskManager> diskManager)
            : m_diskManager{diskManager}, m_frames(numFrames) {
            for (size_t in = 0; in < m_frames.size(); ++in) {
                m_freeFrames.push(in);
            }
        }

        BufferPool::BufferPoolPageGuard BufferPool::GetPage(IoId_t    io_id,
                                                            page_id_t page_id) {
            BufferPoolKey_t k;
            k = static_cast<BufferPoolKey_t>(io_id) << 48;
            k = k | static_cast<uint64_t>(page_id) >> 16;
            spdlog::info("[GetPage]Buffer pool key for {} and {} is: {}", io_id,
                         page_id, k);

            std::shared_lock lock(m_mutex);
            if (auto it = m_map.find(k); it != m_map.end()) {
                spdlog::info("[GetPage] Found frame for key {}", k);
                Frame &f = m_frames[it->second];
                return BufferPoolPageGuard(f);
            }
            lock.unlock();
            // TODO: indicate that the page is being loaded so that anyone else
            // trying on it can piggy back on it.

            // TODO: For now, assume we always have a free frame.
            FrameId_t freeFrameId;
            m_freeFrames.pop(&freeFrameId);
            Frame &f = m_frames[freeFrameId];
            iovec  buffer;
            buffer.iov_base = f.m_page;
            buffer.iov_len  = sizeof(f.m_page) / sizeof(f.m_page[0]);

            if (auto err = readPageFromDisk(io_id, page_id, buffer); err) {
                throw std::runtime_error{fmt::format(
                    "Err in reading page from disk: {}", err.what())};
            }

            lock.lock();
            m_map[k] = freeFrameId;
            return BufferPoolPageGuard(f);
        }

        Error BufferPool::readPageFromDisk(IoId_t io_id, page_id_t page_id,
                                           iovec buffer) const {
            return m_diskManager->read(io_id, page_id * PAGE_SIZE_B, buffer);
        }

        Error BufferPool::flushPage(IoId_t io_id, page_id_t page_id) {
            throw std::runtime_error("not implemented");
        }

        void BufferPool::evictPage() {
            throw std::runtime_error("not implemented");
        }
    } // namespace Core

} // namespace Pig