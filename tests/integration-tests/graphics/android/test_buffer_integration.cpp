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

class AndroidBufferIntegration : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ASSERT_FALSE(mtd::is_surface_flinger_running());
        ASSERT_NO_THROW(
        {
            platform = mg::create_platform(std::make_shared<mg::NullDisplayReport>());
            display = platform->create_display();
        });
    }

    virtual void SetUp()
    {
        ASSERT_TRUE(platform != NULL);
        ASSERT_TRUE(display != NULL);

        auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
        allocator = platform->create_buffer_allocator(buffer_initializer);
        strategy = std::make_shared<mc::SwapperFactory>(allocator);
        size = geom::Size{geom::Width{gl_animation.texture_width()},
                          geom::Height{gl_animation.texture_height()}};
        pf  = geom::PixelFormat::abgr_8888;
        buffer_properties = mc::BufferProperties{size, pf, mc::BufferUsage::software};
    }

    md::glAnimationBasic gl_animation;
    std::shared_ptr<mc::GraphicBufferAllocator> allocator;
    std::shared_ptr<mc::SwapperFactory> strategy;
    geom::Size size;
    geom::PixelFormat pf;
    mc::BufferProperties buffer_properties;
    mtd::TestGrallocMapper sw_renderer;

    /* note about display: android drivers seem to only be able to open fb once
       per process (gralloc's framebuffer_close() doesn't seem to work). once we
       figure out why, we can put display in the test fixture */
    static std::shared_ptr<mg::Platform> platform;
    static std::shared_ptr<mg::Display> display;
};

std::shared_ptr<mg::Display> AndroidBufferIntegration::display;
std::shared_ptr<mg::Platform> AndroidBufferIntegration::platform;

}

TEST_F(AndroidBufferIntegration, post_does_not_throw)
{
    using namespace testing;

    EXPECT_NO_THROW(
    {
        display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
            buffer.post_update();
        });
    });
}

TEST(AndroidBufferIntegrationBasic, alloc_does_not_throw)
{
    using namespace testing;

    EXPECT_NO_THROW({
    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::SwapperFactory>(allocator);
    });

}
TEST_F(AndroidBufferIntegration, swapper_creation_ok)
{
    using namespace testing;

    mc::BufferProperties actual;

    EXPECT_NO_THROW({
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(actual, buffer_properties);
    });
}

TEST_F(AndroidBufferIntegration, swapper_returns_non_null)
{
    using namespace testing;

    mc::BufferProperties actual;
    std::shared_ptr<mc::Buffer> returned_buffer;
    mc::BufferID id{34};

    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(actual, buffer_properties);

    swapper->client_acquire(returned_buffer, id);
    EXPECT_NE(nullptr, returned_buffer);
}

TEST_F(AndroidBufferIntegration, buffer_ok_with_egl_context)
{
    using namespace testing;

    mtd::DrawPatternSolid red_pattern(0xFF0000FF);
    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::SwapperFactory>(allocator);

    geom::PixelFormat pf(geom::PixelFormat::abgr_8888);
    mc::BufferProperties buffer_properties{size, pf, mc::BufferUsage::software};
    mc::BufferProperties actual;
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(actual, buffer_properties);
    auto bundle = std::make_shared<mc::BufferBundleSurfaces>(std::move(swapper));

    gl_animation.init_gl();

    auto client_buffer = bundle->secure_client_buffer();
    auto ipc_package = client_buffer->get_ipc_package();
    auto region = sw_renderer.get_graphic_region_from_package(ipc_package, size);
    red_pattern.draw(region);
    client_buffer.reset();

    auto texture_res = bundle->lock_back_buffer();
    gl_animation.render_gl();
    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
    texture_res.reset();
}

TEST_F(AndroidBufferIntegration, DISABLED_buffer_ok_with_egl_context_repeat)
{
    using namespace testing;

    mtd::DrawPatternSolid red_pattern(0xFF0000FF);
    mtd::DrawPatternSolid green_pattern(0xFF00FF00);

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::SwapperFactory>(allocator);

    geom::PixelFormat pf(geom::PixelFormat::abgr_8888);
    mc::BufferProperties buffer_properties{size, pf, mc::BufferUsage::software};
    mc::BufferProperties actual;
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(actual, buffer_properties);
    auto bundle = std::make_shared<mc::BufferBundleSurfaces>(std::move(swapper));

    gl_animation.init_gl();

    for(;;)
    {
        /* buffer 0 */
        auto client_buffer = bundle->secure_client_buffer();
        auto ipc_package = client_buffer->get_ipc_package();
        auto region = sw_renderer.get_graphic_region_from_package(ipc_package, size);
        red_pattern.draw(region);
        client_buffer.reset();

        auto texture_res = bundle->lock_back_buffer();
        gl_animation.render_gl();
        display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
            buffer.post_update();
        });
        texture_res.reset();
        sleep(1);

        /* buffer 1 */
        client_buffer = bundle->secure_client_buffer();
        ipc_package = client_buffer->get_ipc_package();
        region = sw_renderer.get_graphic_region_from_package(ipc_package, size);
        green_pattern.draw(region);
        client_buffer.reset();

        texture_res = bundle->lock_back_buffer();
        gl_animation.render_gl();
        display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
            buffer.post_update();
        });
        texture_res.reset();
        sleep(1);
    }

}

TEST_F(AndroidBufferIntegration, display_cleanup_ok)
{
    EXPECT_NO_THROW(
    {
        display.reset();
    });
}
