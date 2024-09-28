// TDODO: remove this
#include "disk-manager.h"
#include "heap.h"
#include <memory>

int main() {
    std::shared_ptr<Pig::Core::DiskManager> diskManager =
        std::make_shared<Pig::Core::DiskManager>();
    volatile auto x = Pig::Core::HeapFile::create(diskManager);

    return 0;
}