#ifndef PIG_CORE_LOCK_FREE_STACK_H
#define PIG_CORE_LOCK_FREE_STACK_H

#include "spdlog/spdlog.h"
#include <_types/_uint64_t.h>
#include <atomic>
#include <string>
#include <utility>

namespace Pig {

    namespace Core {

        using NodeAndGen = uint64_t;

        template <typename E> class LockFreeStack {
          public:
            LockFreeStack() : m_top{0} {}

            void push(E e) {
                Node *n = new Node(std::move(e));
                // Get last 48 bits.
                uint64_t pointerValue = reinterpret_cast<uint64_t>(n);

                // get copy of top
                uint64_t top = m_top;
                n->m_prev    = reinterpret_cast<Node *>(top & 0xFFFFFFFFFFFF);

                uint64_t incrementedTop =
                    ((top >> 48) + 1)
                    << 48; // Increment and shift back to upper 16 bits

                // Step 3: Combine the incremented upper 16 bits with the
                // pointer value in the lower 48 bits
                uint64_t newTop = incrementedTop | pointerValue;

                while (!m_top.compare_exchange_strong(top, newTop)) {
                    // If the exchange fails, reload the expected value and
                    // recompute newTop
                    top            = m_top.load();
                    incrementedTop = ((top >> 48) + 1) << 48;
                    newTop         = incrementedTop | pointerValue;
                    n->m_prev = reinterpret_cast<Node *>(top & 0xFFFFFFFFFFFF);
                }
                spdlog::debug("Pushed {} to stack and prev is {}", e,
                              n->m_prev == 0 ? 0 : n->m_prev->m_val);
            }

            bool pop(E *e) {
                // get copy of top
                uint64_t top = m_top;
                Node    *pointerValue =
                    reinterpret_cast<Node *>(top & 0x0000'FFFFFFFFFFFF);

                if (pointerValue == 0) {
                    return false;
                }

                uint64_t incrementedTop =
                    ((top >> 48) + 1)
                    << 48; // Increment and shift back to upper 16 bits

                uint64_t newTop = incrementedTop | reinterpret_cast<uint64_t>(
                                                       pointerValue->m_prev);

                while (!m_top.compare_exchange_strong(top, newTop)) {
                    // If the exchange fails, reload the expected value and
                    // recompute newTop
                    top = m_top.load();

                    pointerValue =
                        reinterpret_cast<Node *>(top & 0x0000'FFFFFFFFFFFF);
                    if (pointerValue == 0) {
                        return false;
                    }
                    incrementedTop = ((top >> 48) + 1) << 48;
                    newTop         = incrementedTop |
                             reinterpret_cast<uint64_t>(pointerValue->m_prev);
                }
                *e = pointerValue->m_val;
                spdlog::debug("Popped {} from stack and top is now {}", *e,
                              pointerValue->m_prev == 0
                                  ? 0
                                  : pointerValue->m_prev->m_val);
                delete pointerValue;
                return true;
            }

          private:
            struct Node {
                E m_val; // delcare copyable and movable and some other things.
                Node *m_prev;

                Node(E val) : m_val{std::move(val)}, m_prev{nullptr} {}
            };
            // Highest 16 bits are generation which wraps around, remaining 48
            // bits for pointer to Node
            std::atomic_uint64_t m_top;
        };
    } // namespace Core

} // namespace Pig

#endif