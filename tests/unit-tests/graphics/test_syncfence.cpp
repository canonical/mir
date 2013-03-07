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

class SyncFenceTest : public ::testing::Test
{
public:
    SyncFenceTest()
    {

    }

    int dummyfd;

};

TEST_F(SyncFenceTest, test_fd_wait)
{
    mga::SyncFence fence(dummyfd);

    EXPECT_CALL(*filedesc_mocker, ioctl(dummyfd, IOCTLNUMFORWAIT ,Lt(0)))
        .Times(1);

    fence->wait();
}

TEST_F(SyncFenceTest, test_fd_destruction_closes_fd)
{
    EXPECT_CALL(*filedesc_mocker, close(dummyfd))
        .Times(1);

    mga::SyncFence fence(dummyfd);
}
