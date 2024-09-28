#include "heap.h"
#include "core.h"
#include "util.h"
#include <memory>

#include <fmt/core.h>
#include <fmt/format.h>

namespace Pig {
    namespace Core {

        HeapFile::HeapFile(std::shared_ptr<DiskManager> diskManager)
            : m_diskManager{std::move(diskManager)} {
            // Use diskManager to intialize new file
            m_id = m_diskManager->registerFile(MAX_PAGES * PAGE_SIZE_KB +
                                               4 /* header + 3 spacemap */ *
                                                   PAGE_SIZE_KB);
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

    } // namespace Core
} // namespace Pig