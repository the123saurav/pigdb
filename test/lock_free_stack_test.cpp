#include "lock_free_stack.h"
#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace Pig {
namespace Core {

// Test fixture for LockFreeStack
class LockFreeStackTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Code here will be called immediately before each test
  }

  void TearDown() override {
    // Code here will be called immediately after each test
  }

  // Helper function to create a stack and push some values
  void fillStack(LockFreeStack<int> &stack) {
    stack.push(10);
    stack.push(20);
    stack.push(30);
  }
};

// Test pushing elements to the stack
TEST_F(LockFreeStackTest, PushElements) {
  LockFreeStack<int> stack;
  stack.push(1);
  stack.push(2);

  int value;
  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 2);

  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 1);

  ASSERT_FALSE(stack.pop(&value)); // Stack should be empty now
}

// Test popping elements from the stack
TEST_F(LockFreeStackTest, PopElements) {
  LockFreeStack<int> stack;
  fillStack(stack);

  int value;
  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 30);

  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 20);

  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 10);

  ASSERT_FALSE(stack.pop(&value)); // Stack should be empty now
}

// Test popping from an empty stack
TEST_F(LockFreeStackTest, PopFromEmptyStack) {
  LockFreeStack<int> stack;
  int value;
  ASSERT_FALSE(stack.pop(&value)); // Should return false on empty stack
}

// Test interleaved push and pop operations
TEST_F(LockFreeStackTest, InterleavedPushAndPop) {
  LockFreeStack<int> stack;

  stack.push(100);
  int value;
  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 100);

  stack.push(200);
  stack.push(300);
  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 300);

  ASSERT_TRUE(stack.pop(&value));
  EXPECT_EQ(value, 200);

  ASSERT_FALSE(stack.pop(&value)); // Stack should be empty now
}

// Test concurrent pushing and popping operations
TEST_F(LockFreeStackTest, ConcurrentPushAndPop) {
  LockFreeStack<int> stack;
  const int numThreads = 4;
  const int numOperationsPerThread = 1000;
  std::atomic<int> totalPoppedElements{0};

  // Function to be run by each thread to push and pop elements
  auto threadFunc = [&stack, numThreads, numOperationsPerThread,
                     &totalPoppedElements](int mul) {
    for (int i = mul; i < numOperationsPerThread * numThreads;
         i += numThreads) {
      stack.push(i); // Push operation
    }

    int value;
    for (int i = mul; i < numOperationsPerThread * numThreads;
         i += numThreads) {
      if (stack.pop(&value)) { // Pop operation
        ++totalPoppedElements; // Track successful pops
      }
    }
  };

  // Create and start threads
  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back(threadFunc, i);
  }

  // Wait for all threads to complete
  for (auto &t : threads) {
    t.join();
  }

  // Verify that all operations were successfully performed
  EXPECT_EQ(totalPoppedElements.load(), numThreads * numOperationsPerThread);
  int remainingValue;
  ASSERT_FALSE(stack.pop(&remainingValue)); // Stack should be empty now
}

} // namespace Core
} // namespace Pig
