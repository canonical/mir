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

#include "src/server/graphics/android/android_platform.h"
#include "src/server/graphics/android/android_buffer_allocator.h"
#include "src/server/graphics/android/android_display.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/null_display_report.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_surfaces.h"
#include "mir/compositor/buffer_properties.h"

#include "mir/draw/graphics.h"
#include "mir_test/draw/android_graphics.h"
#include "mir_test/draw/patterns.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace md=mir::draw;
namespace mtd=mir::test::draw;

namespace
{

class AndroidGraphicsPlatform : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        platform = mg::create_platform(std::make_shared<mg::NullDisplayReport>());
        ASSERT_TRUE(platform != NULL);

        size = geom::Size{geom::Width{334},
                          geom::Height{122}};
        pf  = geom::PixelFormat::abgr_8888;
        buffer_properties = mc::BufferProperties{size, pf, mc::BufferUsage::software};
        null_buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    }

    std::shared_ptr<mg::BufferInitializer> null_buffer_initializer;
    geom::Size size;
    geom::PixelFormat pf;
    mc::BufferProperties buffer_properties;
    mtd::TestGrallocMapper sw_renderer;

    std::shared_ptr<mg::Platform> platform;
};

}

TEST_F(AndroidGraphicsPlatform, allocator_creation_ok)
{
    using namespace testing;

    EXPECT_NO_THROW({
        auto allocator = std::make_shared<mga::AndroidBufferAllocator>(null_buffer_initializer);
    });
}

TEST_F(AndroidGraphicsPlatform, allocator_can_create_sw_buffer)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>(null_buffer_initializer);

    mc::BufferProperties sw_properties{size, pf, mc::BufferUsage::software};
    auto test_buffer = allocator->alloc_buffer(sw_properties);

    auto region = sw_renderer.get_graphic_region_from_package(test_buffer->get_ipc_package(), size);
    mtd::DrawPatternSolid red_pattern(0xFF0000FF);
    red_pattern.draw(region);
    EXPECT_TRUE(red_pattern.check(region));
}

TEST_F(AndroidGraphicsPlatform, allocator_can_create_hw_buffer)
{
    using namespace testing;

    mc::BufferProperties sw_properties{size, pf, mc::BufferUsage::hardware};
    auto allocator = std::make_shared<mga::AndroidBufferAllocator>(null_buffer_initializer);

    //note: it is a bit trickier to test that a gpu can render... just check no throw for now
    EXPECT_NO_THROW({
        auto test_buffer = allocator->alloc_buffer(sw_properties);
    });
}


TEST_F(AndroidGraphicsPlatform, strategy_creation_ok)
{
    using namespace testing;

    EXPECT_NO_THROW({
        auto allocator = std::make_shared<mga::AndroidBufferAllocator>(null_buffer_initializer);
        auto strategy = std::make_shared<mc::SwapperFactory>(allocator);
    });
}

TEST_F(AndroidGraphicsPlatform, swapper_creation_is_sane)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>(null_buffer_initializer);
    auto strategy = std::make_shared<mc::SwapperFactory>(allocator);
    mc::BufferProperties actual;
    mc::BufferID id{34};
    auto swapper = strategy->create_swapper(actual, buffer_properties);
    std::shared_ptr<mc::Buffer> returned_buffer;
    swapper->client_acquire(returned_buffer, id);

    EXPECT_NE(nullptr, returned_buffer);
}
