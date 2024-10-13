#include <sys/_types/_iovec_t.h>
#define UNIT_TEST

// Include Google Test framework
#include <gtest/gtest.h>

// Include necessary headers
#include "core.h"
#include "heap.h"
#include "util.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <sys/uio.h> // For iovec
#include <vector>

using namespace Pig::Core;

HeapFile::Page getEmptyPage(page_id_t pageId) {
  iovec buf;
  buf.iov_base = new unsigned char[PAGE_SIZE_B];
  buf.iov_len = PAGE_SIZE_B;

  HeapFile::Page page(pageId);
  page.initPage(buf);

  PIG_ASSERT(page.getPageId() == pageId, "PageId mismatch");
  PIG_ASSERT(page.getNumSlots() == 0, "Numslots mismatch");
  PIG_ASSERT(page.getFreeBytes() == Pig::Core::HeapFile::Page::FREE_BYTES,
             "freebytes mismatch");
  return page;
}

// Unit tests for the Page class
TEST(PageTest, DefaultConstructor) {
  page_id_t pageId = 1;
  HeapFile::Page page(pageId);

  EXPECT_EQ(pageId, page.getPageId());
  EXPECT_EQ(0, page.getNumSlots());
  EXPECT_EQ(HeapFile::Page::FREE_BYTES, page.getFreeBytes());
  EXPECT_EQ(nullptr, page.getBuffer().iov_base);
}

TEST(PageTest, AddTuple) {

  HeapFile::Page page = getEmptyPage(100);

  // Create a tuple
  const char *data = "test data";
  size_t data_len = strlen(data);
  iovec payload;
  payload.iov_base = const_cast<char *>(data);
  payload.iov_len = data_len;

  uint32_t checksum = 12345; // Arbitrary checksum
  HeapFile::Tuple tuple(checksum, payload);

  // Get initial free bytes and slots
  page_size_t initialFreeBytes = page.getFreeBytes();
  PageSlot initialNumSlots = page.getNumSlots();

  // Add the tuple
  PageSlot slot = page.addTuple(tuple);

  // Verify that slot is 0
  EXPECT_EQ(initialNumSlots, slot);

  // Verify that numSlots is incremented
  EXPECT_EQ(initialNumSlots + 1, page.getNumSlots());

  // Verify that freeBytes is decreased appropriately
  page_size_t expectedFreeBytes =
      initialFreeBytes - HeapFile::Page::spaceForTuple(tuple);
  EXPECT_EQ(expectedFreeBytes, page.getFreeBytes());
}

TEST(PageTest, AddTuplesTillFull) {
  HeapFile::Page page = getEmptyPage(100);

  // Create a tuple
  const char *data = "test data";
  size_t data_len = strlen(data);
  iovec payload;
  payload.iov_base = const_cast<char *>(data);
  payload.iov_len = data_len;

  page_size_t initialFreeBytes = page.getFreeBytes();
  PageSlot initialNumSlots = page.getNumSlots();

  uint32_t checksum = 12345; // Arbitrary checksum
  HeapFile::Tuple tuple(checksum, payload);

  page_size_t spaceForTuple = HeapFile::Page::spaceForTuple(tuple);

  // Calculate how many tuples can fit
  size_t maxTuples = initialFreeBytes / spaceForTuple;

  // Add tuples until the page is full or cannot fit more tuples
  size_t count = 0;
  for (; count < maxTuples; ++count) {
    EXPECT_EQ(page.addTuple(tuple), initialNumSlots++);
  }

  // Verify the number of slots
  EXPECT_EQ(initialNumSlots, page.getNumSlots());

  // Verify that freeBytes is less than one spaceForTuple
  EXPECT_LT(page.getFreeBytes(), spaceForTuple);

  // Attempt to add one more tuple, expect an assertion failure (optional)
  // Note: Since PIG_ASSERT uses assert, it will terminate the program.
  // Therefore, we won't attempt to add another tuple here.
}