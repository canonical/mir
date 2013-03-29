/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/shared/graphics/android/syncfence.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcla = mir::client::android;

namespace
{
class MockIoctlWrapper : public mcla::IoctlWrapper
{
public:
    MOCK_CONST_METHOD3(ioctl, int(int, unsigned long int, int*));
    MOCK_CONST_METHOD1(close, int(int));
};
}

class SyncFenceTest : public ::testing::Test
{
public:
    SyncFenceTest()
     : ioctl_mock(std::make_shared<testing::NiceMock<MockIoctlWrapper>>()),
       dummyfd(44), invalid_fd(-1)
    {
    }

    std::shared_ptr<MockIoctlWrapper> const ioctl_mock;
    int const dummyfd;
    int const invalid_fd;
};

TEST_F(SyncFenceTest, test_valid_fd_wait)
{
    using namespace testing;

    int timeout_val;
    mcla::SyncFence fence(dummyfd, ioctl_mock);
    EXPECT_CALL(*ioctl_mock, ioctl(dummyfd, SYNC_IOC_WAIT, _))
        .Times(1)
        .WillOnce(DoAll(SaveArgPointee<2>(&timeout_val),
                        Return(0)));

    fence.wait();
    EXPECT_GT(0, timeout_val);
}

TEST_F(SyncFenceTest, test_valid_fd_destruction_closes_fd)
{
    EXPECT_CALL(*ioctl_mock, close(dummyfd))
        .Times(1);

    mcla::SyncFence fence(dummyfd, ioctl_mock);
}

TEST_F(SyncFenceTest, test_invalid_fd_does_not_call_ioctl_on_wait)
{
    using namespace testing;

    mcla::SyncFence fence(invalid_fd, ioctl_mock);
    EXPECT_CALL(*ioctl_mock, ioctl(_, _, _))
        .Times(0);

    fence.wait();
}

TEST_F(SyncFenceTest, test_invalid_fd_destruction_does_not_close_fd)
{
    using namespace testing;
    EXPECT_CALL(*ioctl_mock, close(_))
        .Times(0);

    mcla::SyncFence fence(invalid_fd, ioctl_mock);
}
