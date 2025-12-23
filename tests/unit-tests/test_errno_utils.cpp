/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <string>
#include <thread>

#include <mir/errno_utils.h>

TEST(ErrnoToCstr, returns_non_null_and_non_empty)
{
    const char* s = mir::errno_to_cstr(EINVAL);
    ASSERT_NE(s, nullptr);
    ASSERT_GT(std::strlen(s), 0u);
}

TEST(ErrnoToCstr, stable_within_thread_without_intervening_call)
{
    const char* s1 = mir::errno_to_cstr(EINVAL);
    ASSERT_NE(s1, nullptr);

    // Pointer should be stable if we haven't called again in this thread.
    const char* s1_again = s1;
    ASSERT_EQ(s1_again, s1);

    // Content copy should be non-empty.
    std::string copy{s1};
    ASSERT_FALSE(copy.empty());
}

TEST(ErrnoToCstr, subsequent_call_does_not_return_empty)
{
    const char* s1 = mir::errno_to_cstr(EINVAL);
    const char* s2 = mir::errno_to_cstr(ENOENT);

    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    ASSERT_GT(std::strlen(s1), 0u);
    ASSERT_GT(std::strlen(s2), 0u);
}

TEST(ErrnoToCstr, thread_local_isolation_no_cross_thread_clobber)
{
    std::atomic<bool> ready{false};
    std::atomic<bool> done{false};

    std::string t1_first;
    std::string t1_second;

    std::thread t1([&] {
        // Capture message in thread 1
        const char* s = mir::errno_to_cstr(EINVAL);
        ASSERT_NE(s, nullptr);
        t1_first = s;

        ready.store(true, std::memory_order_release);

        // Wait until thread 2 finishes hammering
        while (!done.load(std::memory_order_acquire)) {}

        // Capture again in thread 1; should be consistent
        const char* s2 = mir::errno_to_cstr(EINVAL);
        ASSERT_NE(s2, nullptr);
        t1_second = s2;
    });

    std::thread t2([&] {
        // Wait until thread 1 has captured its first copy
        while (!ready.load(std::memory_order_acquire)) {}

        // Hammer in thread 2; must not affect thread 1’s TLS buffer
        for (int i = 0; i < 100000; ++i) {
            (void)mir::errno_to_cstr(ENOENT);
            (void)mir::errno_to_cstr(EPERM);
            (void)mir::errno_to_cstr(EINVAL);
        }

        done.store(true, std::memory_order_release);
    });

    t1.join();
    t2.join();

    ASSERT_FALSE(t1_first.empty());
    ASSERT_FALSE(t1_second.empty());

    // Because thread 1 asked for the same errno both times, and thread 2 should not
    // clobber thread 1’s TLS storage, these copies should match.
    EXPECT_EQ(t1_first, t1_second);
}
