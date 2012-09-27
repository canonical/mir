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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mock_drm.h"
#include "mock_gbm.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;

class GBMGraphicsPlatform : public ::testing::Test
{
public:
    ::testing::NiceMock<mg::gbm::MockDRM> mock_drm;
    ::testing::NiceMock<mg::gbm::MockGBM> mock_gbm;
};

TEST_F(GBMGraphicsPlatform, get_ipc_package)
{
    using namespace testing;

    /* First time for master DRM fd, second for authenticated fd */
    EXPECT_CALL(mock_drm, drmOpen(_,_))
        .Times(2)
        .WillOnce(Return(mock_drm.fake_drm.fd))
        .WillOnce(Return(66));

    EXPECT_NO_THROW (
        auto platform = mg::create_platform();
        auto pkg = platform->get_ipc_package();

        ASSERT_TRUE(pkg.get());
        ASSERT_EQ(std::vector<int32_t>::size_type{1}, pkg->ipc_fds.size()); 
        ASSERT_EQ(66, pkg->ipc_fds[0]); 
    );
}
