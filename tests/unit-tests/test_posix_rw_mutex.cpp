/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/posix_rw_mutex.h"

#include "mir/test/auto_unblock_thread.h"
#include "mir/test/signal.h"


#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>

#include <gtest/gtest.h>

namespace mt = mir::test;

using namespace std::chrono_literals;

TEST(PosixRWMutex, exclusive_locks_are_exclusive)
{
    mir::PosixRWMutex mutex;

    std::unique_lock<decltype(mutex)> exclusive_lock{mutex};

    bool lock_taken_in_thread{false};
    mt::AutoJoinThread thread_which_should_block{
        [&mutex, &lock_taken_in_thread]()
        {
            std::lock_guard<decltype(mutex)> lock{mutex};
            lock_taken_in_thread = true;
        }};

    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(lock_taken_in_thread);

    exclusive_lock.unlock();
}

TEST(PosixRWMutex, exclusive_trylock_fails_when_exclusive_lock_held)
{
    mir::PosixRWMutex mutex;

    std::lock_guard<decltype(mutex)> exclusive_lock{mutex};

    if (auto second_lock = std::unique_lock<decltype(mutex)>{mutex, std::try_to_lock})
    {
        FAIL() << "Unexpectedly took a second exclusive lock";
    }
}

TEST(PosixRWMutex, read_locks_are_recursive)
{
    mir::PosixRWMutex mutex;

    std::shared_lock<decltype(mutex)> first_lock{mutex};
    std::shared_lock<decltype(mutex)> second_lock{mutex};
}

TEST(PosixRWMutex, exclusive_locks_exclude_read_locks)
{
    mir::PosixRWMutex mutex;

    std::unique_lock<decltype(mutex)> exclusive_lock{mutex};

    bool lock_taken_in_thread{false};
    mt::AutoJoinThread thread_which_should_block{
        [&mutex, &lock_taken_in_thread]()
        {
            std::shared_lock<decltype(mutex)> lock{mutex};
            lock_taken_in_thread = true;
        }};

    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(lock_taken_in_thread);

    exclusive_lock.unlock();
}

TEST(PosixRWMutex, try_shared_lock_fails_when_exclusive_lock_held)
{
    mir::PosixRWMutex mutex;

    std::lock_guard<decltype(mutex)> exclusive_lock{mutex};

    if (auto second_lock = std::shared_lock<decltype(mutex)>{mutex, std::try_to_lock})
    {
        FAIL() << "Unexpectedly took a read lock while write lock was already taken";
    }
}

TEST(PosixRWMutex, read_locks_exclude_exclusive_locks)
{
    mir::PosixRWMutex mutex;

    std::shared_lock<decltype(mutex)> read_lock{mutex};

    bool lock_taken_in_thread{false};
    mt::AutoJoinThread thread_which_should_block{
        [&mutex, &lock_taken_in_thread]()
        {
            std::lock_guard<decltype(mutex)> lock{mutex};
            lock_taken_in_thread = true;
        }};

    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(lock_taken_in_thread);

    read_lock.unlock();
}

TEST(PosixRWMutex, try_exclusive_lock_fails_when_read_lock_held)
{
    mir::PosixRWMutex mutex;

    std::shared_lock<decltype(mutex)> read_lock{mutex};

    if (auto second_lock = std::unique_lock<decltype(mutex)>{mutex, std::try_to_lock})
    {
        FAIL() << "Unexpectedly took a read lock while write lock was already taken";
    }
}

TEST(PosixRWMutex, successful_read_trylock_excludes_write_lock)
{
    mir::PosixRWMutex mutex;

    std::shared_lock<decltype(mutex)> read_lock{mutex, std::try_to_lock};

    bool lock_taken_in_thread{false};
    mt::AutoJoinThread thread_which_should_block{
        [&mutex, &lock_taken_in_thread]()
        {
            std::lock_guard<decltype(mutex)> lock{mutex};
            lock_taken_in_thread = true;
        }};

    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(lock_taken_in_thread);

    read_lock.unlock();
}

TEST(PosixRWMutex, successful_read_trylock_allows_recursive_read_locks)
{
    mir::PosixRWMutex mutex;

    std::shared_lock<decltype(mutex)> read_lock{mutex, std::try_to_lock};

    std::shared_lock<decltype(mutex)> another_read_lock{mutex};
    std::shared_lock<decltype(mutex)> tried_to_read_lock{mutex, std::try_to_lock};

    EXPECT_TRUE(tried_to_read_lock.owns_lock());
}

TEST(PosixRWMutex, successful_trylock_excludes_read_lock)
{
    mir::PosixRWMutex mutex;

    std::unique_lock<decltype(mutex)> exclusive_lock{mutex, std::try_to_lock};

    bool lock_taken_in_thread{false};
    mt::AutoJoinThread thread_which_should_block{
        [&mutex, &lock_taken_in_thread]()
        {
            std::shared_lock<decltype(mutex)> lock{mutex};
            lock_taken_in_thread = true;
        }};

    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(lock_taken_in_thread);

    exclusive_lock.unlock();
}

TEST(PosixRWMutex, successful_trylock_excludes_exclusive_lock)
{
    mir::PosixRWMutex mutex;

    std::unique_lock<decltype(mutex)> exclusive_lock{mutex, std::try_to_lock};

    bool lock_taken_in_thread{false};
    mt::AutoJoinThread thread_which_should_block{
        [&mutex, &lock_taken_in_thread]()
        {
            std::lock_guard<decltype(mutex)> lock{mutex};
            lock_taken_in_thread = true;
        }};

    std::this_thread::sleep_for(1s);
    EXPECT_FALSE(lock_taken_in_thread);

    exclusive_lock.unlock();
}

TEST(PosixRWMutex, prefer_writer_nonrecursive_prevents_writer_starvation)
{
    mir::PosixRWMutex mutex{mir::PosixRWMutex::Type::PreferWriterNonRecursive};

    std::atomic<bool> shutdown_readers{false};

    std::mutex reader_mutex;
    int reader_to_run{0};
    std::condition_variable reader_changed;

    std::array<mt::AutoUnblockThread, 2> readers;
    mt::Signal readers_started;

    for (auto i = 0u; i < readers.size(); ++i)
    {
        readers[i] = mt::AutoUnblockThread{
            [&shutdown_readers]() { shutdown_readers = true; },
            [&](int id)
            {
                auto const trigger_next_reader =
                    [&]()
                    {
                        {
                            std::lock_guard<decltype(reader_mutex)> l{reader_mutex};
                            reader_to_run = (id + 1) % readers.size();
                        }
                        reader_changed.notify_all();
                    };

                while (!shutdown_readers)
                {
                    if (auto lock = std::shared_lock<decltype(mutex)>{mutex, std::try_to_lock})
                    {
                        readers_started.raise();
                        {
                            std::unique_lock<decltype(reader_mutex)> l{reader_mutex};
                            if (reader_to_run != id)
                            {
                                reader_changed.wait(l, [&reader_to_run, id]() { return reader_to_run == id; });
                            }
                        }

                        std::this_thread::yield();

                        trigger_next_reader();
                    }
                    else
                    {
                        // Unable to acquire lock? Either some thread has taken an exclusive lock, or
                        // some thread is waiting on an exclusive lock and we're preempted.
                        //
                        // In the latter case we need to let the waiting thread release its shared lock.
                        trigger_next_reader();
                    }
                }
            },
            i};
    }

    mt::AutoJoinThread watchdog{
        [&]()
        {
            using namespace std::chrono_literals;
            auto timeout = std::chrono::steady_clock::now() + 1min;

            while (!shutdown_readers && (std::chrono::steady_clock::now() < timeout))
            {
                std::this_thread::yield();
            }

            if (!shutdown_readers)
            {
                ADD_FAILURE() << "Timeout waiting to acquire write lock";
            }
            shutdown_readers = true;

            // Ensure that each reader has a chance to run...
            for (auto i = 0u; i < readers.size() ; ++i)
            {
                {
                    std::lock_guard<decltype(reader_mutex)> lock{reader_mutex};
                    reader_to_run = i;
                }
                reader_changed.notify_all();
                readers[i].stop();
            }
        }};

    // Wait until the reader threads have spooled up and taken a shared lock.
    ASSERT_TRUE(readers_started.wait_for(std::chrono::seconds{10}));

    std::lock_guard<decltype(mutex)> lock{mutex};

    shutdown_readers = true;
}
