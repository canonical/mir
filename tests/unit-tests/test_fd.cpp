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

#include <mir/fd.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unistd.h>
#include <fcntl.h>

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

TEST_F(Fd, does_not_close_if_fd_is_borrowed)
{
    EXPECT_TRUE(fd_is_open(raw_fd));
    {
        mir::Fd fd = mir::Fd::borrow(raw_fd);
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

TEST_F(Fd, borrowed_fd_can_be_copied)
{
    EXPECT_TRUE(fd_is_open(raw_fd));
    mir::Fd fd1 = mir::Fd::borrow(raw_fd);
    mir::Fd fd2 = fd1;  // Copy borrowed Fd
    mir::Fd fd3(fd2);   // Copy construct from borrowed Fd

    // All should refer to the same FD value
    EXPECT_EQ(static_cast<int>(fd1), raw_fd);
    EXPECT_EQ(static_cast<int>(fd2), raw_fd);
    EXPECT_EQ(static_cast<int>(fd3), raw_fd);

    // FD should remain open after all borrowed references go out of scope
    fd1 = mir::Fd(-1);
    fd2 = mir::Fd(-1);
    fd3 = mir::Fd(-1);
    EXPECT_TRUE(fd_is_open(raw_fd));
}

TEST_F(Fd, borrowed_fd_converts_to_int)
{
    mir::Fd borrowed = mir::Fd::borrow(raw_fd);
    int extracted_fd = static_cast<int>(borrowed);
    EXPECT_EQ(extracted_fd, raw_fd);
    EXPECT_TRUE(fd_is_open(raw_fd));
}
