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
#include "mir_test_doubles/mock_fence.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

//FIXME: (lp-1229884) this ioctl code should be taken from kernel headers
#define SYNC_IOC_WAIT 0x40043E00
#define SYNC_IOC_MERGE 0x444

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
        mock_fops = std::make_shared<MockFileOps>();
    }

    int const dummy_fd = 3;
    int const invalid_fd = -1;
    std::shared_ptr<MockFileOps> mock_fops;
};


TEST_F(SyncSwTest, sync_wait)
{
    EXPECT_CALL(*mock_fops, ioctl(dummy_fd, SYNC_IOC_WAIT, (void*)-1))
        .Times(1);
    mga::SyncFence fence1(mock_fops, dummy_fd);
    fence1.wait();

    //will not call ioctl
    mga::SyncFence fence2(mock_fops, invalid_fd);
    fence2.wait();
}

TEST_F(SyncSwTest, sync_merge_with_valid_fd)
{
    using namespace testing;
    EXPECT_CALL(*mock_fops, ioctl(dummy_fd, SYNC_IOC_MERGE, _))
        .Times(1);

    mga::SyncFence fence1(mock_fops, dummy_fd);
    mga::SyncFence fence2(mock_fops, dummy_fd);
    mga::SyncFence fence3(mock_fops, invalid_fd);

    fence1.merge_with(std::move(fence2));
    fence1.merge_with(std::move(fence3));
}

TEST_F(SyncSwTest, sync_merge_with_invalid_fd)
{
    using namespace testing;
    EXPECT_CALL(*mock_fops, ioctl(dummy_fd, SYNC_IOC_MERGE, _))
        .Times(0);

    mga::SyncFence fence1(mock_fops, invalid_fd);
    mga::SyncFence fence2(mock_fops, invalid_fd);
    mga::SyncFence fence3(mock_fops, dummy_fd);

    fence1.merge_with(std::move(fence2));
    fence1.merge_with(std::move(fence3));
}

TEST_F(SyncSwTest, extract_dups_fd)
{
    using namespace testing;
    int fd2 = dummy_fd + 1;
    EXPECT_CALL(*mock_fops, dup(dummy_fd))
        .Times(0)
        .WillOnce(Return(fd2));;

    mga::SyncFence fence(mock_fops, dummy_fd);

    EXPECT_EQ(fd2, fence.extract_native_handle());
}


