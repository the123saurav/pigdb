#ifndef PIG_CORE_LOCK_FREE_STACK_H
#define PIG_CORE_LOCK_FREE_STACK_H

#include "spdlog/spdlog.h"
#include <atomic>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace Pig {

    namespace Core {

        using NodeAndGen = uint64_t;

        /**
            The first 16 bits store a monotonic wrapping int
            for ABA problem preventing updating top with wrong
            value.
         */
        template <typename E> class LockFreeStack {
            static_assert(std::is_move_constructible_v<E>,
                          "E must be move constructible.");
            static_assert(std::is_move_assignable_v<E>,
                          "E must be move assignable.");

            static_assert(std::is_nothrow_copy_constructible_v<E>,
                          "E must have a noexcept copy constructor.");
            static_assert(std::is_nothrow_copy_assignable_v<E>,
                          "E must have a noexcept copy assignment operator.");

          public:
            LockFreeStack() : m_top{0} {}

            void push(E e) {
                // Okay to fail at allocation.
                auto     n            = std::make_unique<Node>(std::move(e));
                uint64_t pointerValue = reinterpret_cast<uint64_t>(n.get());

                // get copy of top
                uint64_t top = m_top;
                n->m_prev    = reinterpret_cast<Node *>(top & 0xFFFFFFFFFFFF);

                uint16_t incrementedTop =
                    (static_cast<uint16_t>(top >> 48) +
                     1); // Increment and shift back to upper 16 bits

                // Step 3: Combine the incremented upper 16 bits with the
                // pointer value in the lower 48 bits
                uint64_t newTop =
                    static_cast<uint64_t>(incrementedTop) << 48 | pointerValue;

                while (!m_top.compare_exchange_strong(top, newTop)) {
                    // If the exchange fails, reload the expected value and
                    // recompute newTop
                    top            = m_top.load();
                    incrementedTop = static_cast<uint16_t>(top >> 48) + 1;
                    newTop = static_cast<uint64_t>(incrementedTop) << 48 |
                             pointerValue;
                    n->m_prev = reinterpret_cast<Node *>(top & 0xFFFFFFFFFFFF);
                }

                spdlog::debug("Pushed {} to stack and prev is {}", e,
                              n->m_prev == 0 ? 0 : n->m_prev->m_val);
                n.release(); // Managed by stack now
            }

            bool pop(E *e) {
                // get copy of top
                uint64_t top = m_top;
                Node    *pointerValue =
                    reinterpret_cast<Node *>(top & 0x0000'FFFFFFFFFFFF);

                if (pointerValue == 0) {
                    return false;
                }

                uint16_t incrementedTop =
                    static_cast<uint16_t>(top >> 48) +
                    1; // Increment and shift back to upper 16 bits

                uint64_t newTop =
                    static_cast<uint64_t>(incrementedTop) << 48 |
                    reinterpret_cast<uint64_t>(pointerValue->m_prev);

                while (!m_top.compare_exchange_strong(top, newTop)) {
                    // If the exchange fails, reload the expected value and
                    // recompute newTop
                    top = m_top.load();

                    pointerValue =
                        reinterpret_cast<Node *>(top & 0x0000'FFFFFFFFFFFF);
                    if (pointerValue == 0) {
                        return false;
                    }
                    incrementedTop = static_cast<uint16_t>(top >> 48) + 1;
                    newTop = static_cast<uint64_t>(incrementedTop) << 48 |
                             reinterpret_cast<uint64_t>(pointerValue->m_prev);
                }
                // Use caller supplied memory as we do not want to fail now as
                // we have popped, hence no allocation.
                // Note that copy assignment cant throw here
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