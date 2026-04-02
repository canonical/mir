/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/platforms/evdev/fd_store.h"

#include <mir/fd.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fcntl.h>

namespace mie = mir::input::evdev;

namespace
{

// Open /dev/null to obtain a real, valid file descriptor for testing.
mir::Fd make_fd()
{
    return mir::Fd{::open("/dev/null", O_RDWR | O_CLOEXEC)};
}

int take_fd_raw(mie::FdStore& store, char const* path)
{
    return static_cast<int>(store.take_fd(path));
}

}

TEST(FdStore, stores_and_retrieves_fd)
{
    mie::FdStore store;
    auto fd = make_fd();
    int const raw = static_cast<int>(fd);

    store.store_fd("/dev/input/event0", std::move(fd));

    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(raw));
}

TEST(FdStore, returns_invalid_fd_for_unknown_path)
{
    mie::FdStore store;

    EXPECT_THAT(take_fd_raw(store, "/dev/input/event99"), testing::Eq(mir::Fd::invalid));
}

TEST(FdStore, multiple_concurrent_suspends_are_all_tracked)
{
    mie::FdStore store;

    auto fd0 = make_fd();
    auto fd1 = make_fd();
    auto fd2 = make_fd();
    int const raw0 = static_cast<int>(fd0);
    int const raw1 = static_cast<int>(fd1);
    int const raw2 = static_cast<int>(fd2);

    store.store_fd("/dev/input/event0", std::move(fd0));
    store.store_fd("/dev/input/event1", std::move(fd1));
    store.store_fd("/dev/input/event2", std::move(fd2));

    // Suspend all three
    store.remove_fd(raw0);
    store.remove_fd(raw1);
    store.remove_fd(raw2);

    // All three should be recoverable via take_fd
    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(raw0));
    EXPECT_THAT(take_fd_raw(store, "/dev/input/event1"), testing::Eq(raw1));
    EXPECT_THAT(take_fd_raw(store, "/dev/input/event2"), testing::Eq(raw2));
}

TEST(FdStore, take_fd_reinstates_correct_fd_per_path)
{
    mie::FdStore store;

    auto fd_a = make_fd();
    auto fd_b = make_fd();
    int const raw_a = static_cast<int>(fd_a);
    int const raw_b = static_cast<int>(fd_b);

    store.store_fd("/dev/input/event0", std::move(fd_a));
    store.store_fd("/dev/input/event1", std::move(fd_b));

    // Suspend only event0
    store.remove_fd(raw_a);

    // event1 is still active; event0 is suspended but should be returned
    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(raw_a));
    EXPECT_THAT(take_fd_raw(store, "/dev/input/event1"), testing::Eq(raw_b));
}

TEST(FdStore, purge_fd_prevents_reinstatement_from_suspended)
{
    mie::FdStore store;

    auto fd = make_fd();
    int const raw = static_cast<int>(fd);

    store.store_fd("/dev/input/event0", std::move(fd));
    store.remove_fd(raw);

    store.purge_fd("/dev/input/event0");

    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(mir::Fd::invalid));
}

TEST(FdStore, purge_fd_also_removes_from_active_fds)
{
    mie::FdStore store;

    auto fd = make_fd();
    store.store_fd("/dev/input/event0", std::move(fd));

    // purge without suspending first (defensive use of purge_fd)
    store.purge_fd("/dev/input/event0");

    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(mir::Fd::invalid));
}

TEST(FdStore, second_suspend_of_same_path_overwrites_stale_cached_fd)
{
    mie::FdStore store;

    // Simulate a double-suspend edge case: store, suspend, re-store, re-suspend.
    auto fd1 = make_fd();
    int const raw1 = static_cast<int>(fd1);

    store.store_fd("/dev/input/event0", std::move(fd1));
    store.remove_fd(raw1); // fd1 is now in suspended

    // A new FD arrives at the same path (e.g. after a resume + immediate re-suspend)
    auto fd2 = make_fd();
    int const raw2 = static_cast<int>(fd2);
    store.store_fd("/dev/input/event0", std::move(fd2));
    store.remove_fd(raw2); // should overwrite fd1 in suspended with fd2

    // take_fd should return the newer fd2, not the stale fd1
    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(raw2));
}
