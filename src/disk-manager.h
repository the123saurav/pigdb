#ifndef PIG_CORE_DISK_MANAGER_H
#define PIG_CORE_DISK_MANAGER_H

#include "core.h"
#include "error.h"
#include "util.h"
#include <_types/_uint16_t.h>
#include <_types/_uint64_t.h>
#include <array>
#include <atomic>
#include <mutex>
#include <sys/_types/_iovec_t.h>

namespace Pig {
    namespace Core {

        using IoId_t = uint16_t;

        /**
        A Disk Manager allows doing disk IO at unit of page size.
        It also manages how many pages are there in the file backing it,
        which is initialized at time of creation and can grow with usage.

        It does not understand the contents of any page.

        Currently it is implemented as in memory buffer, in future
        we can have one DiskManager per disk device.
         */
        class DiskManager {
          public:
            DiskManager();

            /**
                All entities who want to do IO need to register themselves
                at creation.
                Additionally in future, disk manager would scan the db dir
                to figure out the entities based on extension and register them
                at time of recovery.

                The registered id never changes for an entity and hence is part
               of the name.
             */
            IoId_t registerFile(uint64_t initalSizeBytes);

            [[nodiscard]] Error read(IoId_t id, uint64_t offset,
                                     iovec buffer) const;

            [[nodiscard]] Error write(IoId_t id, uint64_t offset, iovec buffer);

          private:
            std::mutex m_lock;
            // OwningIovec         *m_buffers;
            std::unique_ptr<OwningIovec[]> m_buffers;
            std::atomic_uint16_t           m_size{0};
        };
    } // namespace Core
} // namespace Pig

#endif