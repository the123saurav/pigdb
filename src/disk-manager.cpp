#include "error.h"
#include "fmt/core.h"
#include "fmt/format.h"

#include "core.h"
#include "disk-manager.h"
#include "util.h"
#include <atomic>
#include <cstring>
#include <mutex>
#include <new>
#include <sys/_types/_iovec_t.h>

namespace Pig {

    namespace Core {
        DiskManager::DiskManager()
            : m_buffers{std::make_unique<OwningIovec[]>(MAX_TABLES)} {
            memset(m_buffers.get(), 0, MAX_TABLES * sizeof(OwningIovec));
        }

        IoId_t DiskManager::registerFile(uint64_t initalSizeBytes) {
            IoId_t id           = m_size.fetch_add(1);
            m_buffers.get()[id] = OwningIovec(initalSizeBytes);

            return id;
        }

        Error DiskManager::read(IoId_t id, uint64_t offset,
                                iovec buffer) const {
            PIG_ASSERT(id < m_size.load(std::memory_order_acquire),
                       "Bad id for read");
            return m_buffers.get()[id].read(offset, buffer);
        }

        Error DiskManager::write(IoId_t id, uint64_t offset, iovec buffer) {
            PIG_ASSERT(id < m_size.load(std::memory_order_acquire),
                       "Bad id for write");
            return m_buffers.get()[id].write(offset, buffer);
        }
    } // namespace Core

} // namespace Pig