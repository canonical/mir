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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#ifndef ANDROID
#include "gbm/mock_drm.h"
#include "gbm/mock_gbm.h"
#endif

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

class GraphicsPlatform : public ::testing::Test
{
#ifndef ANDROID
public:
    GraphicsPlatform()
    {
        using namespace testing;

        ON_CALL(mock_gbm, gbm_bo_get_width(_))
        .WillByDefault(Return(320));

        ON_CALL(mock_gbm, gbm_bo_get_height(_))
        .WillByDefault(Return(240));
    }

    ::testing::NiceMock<mg::gbm::MockDRM> mock_drm;
    ::testing::NiceMock<mg::gbm::MockGBM> mock_gbm;
#endif
};

TEST_F(GraphicsPlatform, buffer_allocator_creation)
{
    using namespace testing;

    EXPECT_NO_THROW (
        auto platform = mg::create_platform();
        auto allocator = mg::create_buffer_allocator(platform);

        EXPECT_TRUE(allocator.get());
    );

}

TEST_F(GraphicsPlatform, buffer_creation)
{
    auto platform = mg::create_platform();
    auto allocator = mg::create_buffer_allocator(platform);
    geom::Width w(320);
    geom::Height h(240);
    mc::PixelFormat pf(mc::PixelFormat::rgba_8888);
    auto buffer = allocator->alloc_buffer(w, h, pf);

    ASSERT_TRUE(buffer.get() != NULL); 

    EXPECT_EQ(buffer->width(), w);
    EXPECT_EQ(buffer->height(), h);
    EXPECT_EQ(buffer->pixel_format(), pf );
 
}
