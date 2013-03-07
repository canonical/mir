/*
 * Copyright © 2012 Canonical Ltd.
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

#include "src/client/android/syncfence.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcla = mir::client::android;

namespace
{
class MockIoctlWrapper : public mcla::IoctlWrapper
{
public:
    MOCK_METHOD3(ioctl, int(int, unsigned long int, int));
    MOCK_METHOD1(close, int(int));
};
}

class SyncFenceTest : public ::testing::Test
{
public:
    SyncFenceTest()
     : ioctl_mock(std::make_shared<MockIoctlWrapper>()),
       dummyfd(44)
    {
    }

    std::shared_ptr<MockIoctlWrapper> ioctl_mock;
    int dummyfd;
};

TEST_F(SyncFenceTest, test_fd_wait)
{
    mcla::SyncFence fence(dummyfd, ioctl_mock);

    EXPECT_CALL(*ioctl_mock, ioctl(dummyfd, 44 , testing::Lt(0)))
        .Times(1);

    fence.wait();
}

TEST_F(SyncFenceTest, test_fd_destruction_closes_fd)
{
    EXPECT_CALL(*ioctl_mock, close(dummyfd))
        .Times(1);

    mcla::SyncFence fence(dummyfd, ioctl_mock);
}
