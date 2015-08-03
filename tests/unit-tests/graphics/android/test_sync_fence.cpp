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

#include "mir/graphics/android/sync_fence.h"
#include "mir/test/doubles/mock_fence.h"

#include <android/linux/sync.h>
#include <sys/ioctl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

namespace mga = mir::graphics::android;
namespace mtd = mir::test::doubles;

namespace
{
struct MockFileOps : public mga::SyncFileOps
{
    MOCK_METHOD3(ioctl, int(int,int,void*));
    MOCK_METHOD1(dup, int(int));
    MOCK_METHOD1(close, int(int));
};
}

class SyncSwTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_fops = std::make_shared<testing::NiceMock<MockFileOps>>();
        dummy_fd_value = 3;
    }

    int dummy_fd_value{3};
    int invalid_fd_value{-1};
    mir::Fd dummy_fd{std::move(dummy_fd_value)};
    mir::Fd invalid_fd{std::move(invalid_fd_value)};
    std::shared_ptr<MockFileOps> mock_fops;
};


MATCHER_P(TimeoutMatches, value,
          std::string("timeout should be: " + testing::PrintToString(value)))
{
    int* timeout = static_cast<int*>(arg);
    if (!timeout)
        return false;
    return value == *timeout;
}

MATCHER_P(MergeMatches, value,
          std::string("merge should be: " + testing::PrintToString(value)))
{
    auto argument = static_cast<struct sync_merge_data*>(arg);
    return argument->fd2 == value.fd2;
}

TEST_F(SyncSwTest, sync_wait)
{
    EXPECT_CALL(*mock_fops, ioctl(dummy_fd_value, SYNC_IOC_WAIT, TimeoutMatches(-1)))
        .Times(1);
    mga::SyncFence fence1(mock_fops, std::move(dummy_fd));
    fence1.wait();

    //will not call ioctl
    mga::SyncFence fence2(mock_fops, std::move(invalid_fd));
    fence2.wait();
}

namespace
{
struct IoctlSetter
{
    IoctlSetter(int fd)
        : fd(fd)
    {
    }
    int merge_setter(int, int, void* data)
    {
        auto b = static_cast<struct sync_merge_data*>(data);
        b->fence = fd;
        return 0;
    }
    int fd;
};
}
TEST_F(SyncSwTest, sync_merge_with_valid_fd)
{
    using namespace testing;
    int dummy_fd2 = 44;
    int out_fd = 88;
    IoctlSetter setter(out_fd);

    struct sync_merge_data expected_data_in { dummy_fd2, "name", 0 };

    EXPECT_CALL(*mock_fops, ioctl(dummy_fd_value, static_cast<int>(SYNC_IOC_MERGE), MergeMatches(expected_data_in)))
        .Times(1)
        .WillOnce(Invoke(&setter, &IoctlSetter::merge_setter));

    mga::SyncFence fence1(mock_fops, std::move(dummy_fd));
    fence1.merge_with(dummy_fd2);
}

TEST_F(SyncSwTest, sync_merge_with_invalid_fd)
{
    using namespace testing;
    EXPECT_CALL(*mock_fops, ioctl(dummy_fd_value, static_cast<int>(SYNC_IOC_MERGE), _))
        .Times(0);

    mga::SyncFence fence1(mock_fops, std::move(dummy_fd));
    fence1.merge_with(invalid_fd_value);
}

TEST_F(SyncSwTest, copy_dups_fd)
{
    using namespace testing;
    int fd2 = dummy_fd_value + 1;
    EXPECT_CALL(*mock_fops, dup(dummy_fd_value))
        .Times(1)
        .WillOnce(Return(fd2));;

    mga::SyncFence fence(mock_fops, std::move(dummy_fd));

    EXPECT_EQ(fd2, fence.copy_native_handle());
}
