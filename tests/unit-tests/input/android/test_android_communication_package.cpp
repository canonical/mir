/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/android/android_input_channel.h"
#include "mir_test_doubles/mock_surface_info.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <unistd.h>
#include <fcntl.h>

namespace droidinput = android;
namespace mia = mir::input::android;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

struct AndroidInputChannel : public testing::Test
{
    void SetUp()
    {
        surface_info = std::make_shared<mtd::MockSurfaceInfo>();
    }

    std::shared_ptr<mtd::MockSurfaceInfo> surface_info;
};

TEST_F(AndroidInputChannel, packages_own_valid_fds)
{

    int server_fd, client_fd;
    {
        mia::AndroidInputChannel package(surface_info);

        server_fd = package.server_fd();
        client_fd = package.client_fd();
        EXPECT_GT(server_fd, 0);
        EXPECT_GT(client_fd, 0);

        EXPECT_EQ(fcntl(server_fd, F_GETFD), 0);
        EXPECT_EQ(fcntl(client_fd, F_GETFD), 0);
    }
    EXPECT_LT(fcntl(server_fd, F_GETFD), 0);
    EXPECT_LT(fcntl(client_fd, F_GETFD), 0);
}

TEST_F(AndroidInputChannel, uses_topleft_info)
{
    using namespace testing;
    geom::Point pt{geom::X{4}, geom::Y{44}};
    EXPECT_CALL(*surface_info, top_left())
        .Times(1)
        .WillOnce(Return(pt));

    mia::AndroidInputChannel package(surface_info);
    EXPECT_EQ(pt, package.top_left());
}

TEST_F(AndroidInputChannel, uses_size_info)
{
    using namespace testing;
    geom::Size size{geom::Width{4}, geom::Height{44}};
    EXPECT_CALL(*surface_info, size())
        .Times(1)
        .WillOnce(Return(size));

    mia::AndroidInputChannel package(surface_info);
    EXPECT_EQ(size, package.size());
}

TEST_F(AndroidInputChannel, uses_name)
{
    using namespace testing;
    std::string str("coffee");
    EXPECT_CALL(*surface_info, name())
        .Times(1)
        .WillOnce(ReturnRef(str));

    mia::AndroidInputChannel package(surface_info);
    EXPECT_EQ(str, package.name());
}
