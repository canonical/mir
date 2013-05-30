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

#include "mir/compositor/buffer_bundle_surfaces.h"

#include "mir_test_doubles/mock_swapper_master.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/gmock_fixes.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace ms = mir::surfaces;

class BufferBundleTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_buffer = std::make_shared<mtd::StubBuffer>();
        mock_swapper = std::unique_ptr<mtd::MockSwapperMaster>(
            new testing::NiceMock<mtd::MockSwapperMaster>());
    }

    std::shared_ptr<mtd::StubBuffer> mock_buffer;
    std::unique_ptr<mtd::MockSwapperMaster> mock_swapper;
};

TEST_F(BufferBundleTest, get_buffer_for_compositor_handles_resources)
{
    using namespace testing;

    EXPECT_CALL(*mock_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer));
    EXPECT_CALL(*mock_swapper, compositor_release(_))
        .Times(1);

    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper));

    buffer_bundle.lock_back_buffer();
}

TEST_F(BufferBundleTest, get_buffer_for_compositor_can_lock)
{
    using namespace testing;

    EXPECT_CALL(*mock_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer));
    EXPECT_CALL(*mock_swapper, compositor_release(_))
        .Times(1);

    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper));

    buffer_bundle.lock_back_buffer();
}

TEST_F(BufferBundleTest, get_buffer_for_client_releases_resources)
{
    using namespace testing;

    EXPECT_CALL(*mock_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer));
    EXPECT_CALL(*mock_swapper, client_release(_))
        .Times(1);
    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper));

    buffer_bundle.secure_client_buffer();
}
