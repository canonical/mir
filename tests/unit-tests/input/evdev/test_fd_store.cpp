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

TEST(FdStore, remove_fd_closes_the_fd_so_take_fd_returns_invalid)
{
    mie::FdStore store;

    auto fd = make_fd();
    int const raw = static_cast<int>(fd);

    store.store_fd("/dev/input/event0", std::move(fd));
    store.remove_fd(raw);

    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(mir::Fd::invalid));
}

TEST(FdStore, remove_fd_does_not_affect_other_paths)
{
    mie::FdStore store;

    auto fd_a = make_fd();
    auto fd_b = make_fd();
    int const raw_a = static_cast<int>(fd_a);
    int const raw_b = static_cast<int>(fd_b);

    store.store_fd("/dev/input/event0", std::move(fd_a));
    store.store_fd("/dev/input/event1", std::move(fd_b));

    store.remove_fd(raw_a);

    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(mir::Fd::invalid));
    EXPECT_THAT(take_fd_raw(store, "/dev/input/event1"), testing::Eq(raw_b));
}

TEST(FdStore, new_fd_can_be_stored_at_same_path_after_removal)
{
    mie::FdStore store;

    auto fd1 = make_fd();
    int const raw1 = static_cast<int>(fd1);

    store.store_fd("/dev/input/event0", std::move(fd1));
    store.remove_fd(raw1);

    // Simulate resume: a fresh fd arrives at the same path
    auto fd2 = make_fd();
    int const raw2 = static_cast<int>(fd2);
    store.store_fd("/dev/input/event0", std::move(fd2));

    EXPECT_THAT(take_fd_raw(store, "/dev/input/event0"), testing::Eq(raw2));
}
