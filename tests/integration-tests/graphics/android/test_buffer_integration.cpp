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

#include "src/server/graphics/android/android_graphic_buffer_allocator.h"
#include "src/server/compositor/switching_bundle.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/null_display_report.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/graphics/buffer_properties.h"

#include "mir_test/draw/android_graphics.h"
#include "mir_test/draw/patterns.h"

#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace mtd=mir::test::draw;

namespace
{

class AndroidBufferIntegration : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        size = geom::Size{334, 122};
        pf  = geom::PixelFormat::abgr_8888;
        buffer_properties = mg::BufferProperties{size, pf, mg::BufferUsage::software};
        null_buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    }

    std::shared_ptr<mg::BufferInitializer> null_buffer_initializer;
    geom::Size size;
    geom::PixelFormat pf;
    mg::BufferProperties buffer_properties;
    mtd::TestGrallocMapper sw_renderer;
};

}

TEST_F(AndroidBufferIntegration, allocator_can_create_sw_buffer)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(null_buffer_initializer);

    mg::BufferProperties sw_properties{size, pf, mg::BufferUsage::software};
    auto test_buffer = allocator->alloc_buffer(sw_properties);

    auto region = sw_renderer.graphic_region_from_handle(test_buffer->native_buffer_handle()->anwb());
    mtd::DrawPatternSolid red_pattern(0xFF0000FF);
    red_pattern.draw(*region);
    EXPECT_TRUE(red_pattern.check(*region));
}

TEST_F(AndroidBufferIntegration, allocator_can_create_hw_buffer)
{
    using namespace testing;

    mg::BufferProperties hw_properties{size, pf, mg::BufferUsage::hardware};
    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(null_buffer_initializer);

    //TODO: kdub it is a bit trickier to test that a gpu can render... just check creation for now
    auto test_buffer = allocator->alloc_buffer(hw_properties);
    EXPECT_NE(nullptr, test_buffer);
}

TEST_F(AndroidBufferIntegration, swapper_creation_is_sane)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(null_buffer_initializer);

    mc::SwitchingBundle swapper(2, allocator, buffer_properties);

    auto returned_buffer = swapper.client_acquire();

    EXPECT_NE(nullptr, returned_buffer);
}
