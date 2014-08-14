/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/fd.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

namespace
{

bool fd_is_open(int fd)
{
    return fcntl(fd, F_GETFD) != -1;
}

struct Fd : public testing::Test
{
    Fd() :
        raw_fd(::open("/dev/null", 0))
    {
    }

    ~Fd()
    {
        if (fd_is_open(raw_fd))
            ::close(raw_fd);
    }

    int const raw_fd;
};
}

TEST_F(Fd, does_not_close_if_construction_doesnt_intend_to_transfer_ownership)
{
    EXPECT_TRUE(fd_is_open(raw_fd));
    {
        mir::Fd fd(mir::IntOwnedFd{raw_fd});
    }
    EXPECT_TRUE(fd_is_open(raw_fd));
}

TEST_F(Fd, closes_when_refcount_is_zero)
{
    EXPECT_TRUE(fd_is_open(raw_fd));
    mir::Fd fd2(-1);
    {
        mir::Fd fd(raw_fd);
        {
            mir::Fd fd1(fd);
            fd2 = fd1;
        }
    }

    EXPECT_TRUE(fd_is_open(raw_fd));
    fd2 = mir::Fd(-1);
    EXPECT_FALSE(fd_is_open(raw_fd));
}

TEST_F(Fd, moves_around)
{
    EXPECT_TRUE(fd_is_open(raw_fd));
    mir::Fd fd0(-1);
    fd0 = mir::Fd(raw_fd);
    mir::Fd fd1(std::move(fd0));
    mir::Fd fd2(fd1);

    EXPECT_TRUE(fd_is_open(raw_fd));
    fd1 = mir::Fd(-1);
    EXPECT_TRUE(fd_is_open(raw_fd));
    fd2 = mir::Fd(-1);
    EXPECT_FALSE(fd_is_open(raw_fd));
}
