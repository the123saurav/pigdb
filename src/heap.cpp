#include "heap.h"
#include "util.h"
#include <memory>

#include <fmt/core.h>
#include <fmt/format.h>

namespace Pig {
    namespace Core {

        HeapFile::HeapFile() {
            void *raw = ::operator new(MAX_PAGES * sizeof(Page));
            m_pages   = reinterpret_cast<Page *>(raw);

            for (std::size_t i = 0; i < MAX_PAGES; ++i) {
                new (&m_pages[i]) Page(i);
            }
        }

        HeapFile::~HeapFile() {
            delete[] m_pages;
        }

        std::unique_ptr<HeapFile> HeapFile::create() {
            return std::unique_ptr<HeapFile>(new HeapFile());
        }

        HeapFile::Page *HeapFile::getPage(PageId pageId) const noexcept {
            PIG_ASSERT(pageId < MAX_PAGES,
                       fmt::format("Invalid PageId {} requested", pageId));

            return m_pages + pageId;
        }

    } // namespace Core
} // namespace Pig