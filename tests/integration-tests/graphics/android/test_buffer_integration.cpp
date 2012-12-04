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

#include "src/graphics/android/android_platform.h"
#include "src/graphics/android/android_buffer_allocator.h"
#include "src/graphics/android/android_display.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_surfaces.h"
#include "mir/compositor/buffer_properties.h"

#include "mir/draw/android_graphics.h"
#include "mir/draw/graphics.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mc=mir::compositor;
namespace geom=mir::geometry; 
namespace mga=mir::graphics::android; 
namespace mg=mir::graphics; 
namespace md=mir::draw;

namespace
{

/* todo: consolidate into common file for rendering and checking */
void render_solid_pattern(const std::shared_ptr<MirGraphicsRegion> &region, uint32_t color)
{
    if (region->pixel_format != mir_pixel_format_rgba_8888 )
        throw(std::runtime_error("error, wrong pixel format for render_solid_pattern"));

    uint32_t *pixel = (uint32_t*) region->vaddr;
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            pixel[j*region->width + i] = color;
        }
    }
}

class AndroidBufferIntegration : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ASSERT_NO_THROW(
        {
            platform = mg::create_platform();
            display = platform->create_display();
        }); 
    }

    virtual void SetUp()
    {
        ASSERT_TRUE(platform != NULL);
        ASSERT_TRUE(display != NULL);
        
        auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
        allocator = platform->create_buffer_allocator(buffer_initializer);
        strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);
        size = geom::Size{geom::Width{gl_animation.texture_width()},
                          geom::Height{gl_animation.texture_height()}};
        pf  = geom::PixelFormat::rgba_8888;
        buffer_properties = mc::BufferProperties{size, pf, mc::BufferUsage::software};
    }

    md::glAnimationBasic gl_animation;
    std::shared_ptr<mc::GraphicBufferAllocator> allocator;
    std::shared_ptr<mc::DoubleBufferAllocationStrategy> strategy;
    geom::Size size;
    geom::PixelFormat pf;
    mc::BufferProperties buffer_properties;
    md::grallocRenderSW sw_renderer;

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
        display->post_update();
    });
}

TEST(AndroidBufferIntegrationBasic, alloc_does_not_throw)
{
    using namespace testing;

    EXPECT_NO_THROW({ 
    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);
    });

}

TEST_F(AndroidBufferIntegration, swapper_creation_ok)
{
    using namespace testing;

    EXPECT_NO_THROW({ 
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(buffer_properties);
    });
}

TEST_F(AndroidBufferIntegration, swapper_returns_non_null)
{
    using namespace testing;

    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(buffer_properties);

    EXPECT_NE((int)swapper->client_acquire(), NULL);
}

TEST_F(AndroidBufferIntegration, buffer_ok_with_egl_context)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);

    geom::PixelFormat pf(geom::PixelFormat::rgba_8888);
    mc::BufferProperties buffer_properties{size, pf, mc::BufferUsage::software};
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(buffer_properties);
    auto generator = std::make_shared<mc::BufferIDMonotonicIncreaseGenerator>();
    auto bundle = std::make_shared<mc::BufferBundleSurfaces>(std::move(swapper), generator);

    gl_animation.init_gl();

    std::shared_ptr<mc::GraphicRegion> texture_res;

    auto client_buffer = bundle->secure_client_buffer();
    auto region = sw_renderer.get_graphic_region_from_package(client_buffer->ipc_package, size);
    render_solid_pattern(region, 0xFF0000FF);
    client_buffer.reset();

    texture_res = bundle->lock_back_buffer();
    gl_animation.render_gl();
    display->post_update();
    texture_res.reset();
}

TEST_F(AndroidBufferIntegration, DISABLED_buffer_ok_with_egl_context_repeat)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);

    geom::PixelFormat pf(geom::PixelFormat::rgba_8888);
    mc::BufferProperties buffer_properties{size, pf, mc::BufferUsage::software};
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(buffer_properties);
    auto generator = std::make_shared<mc::BufferIDMonotonicIncreaseGenerator>();
    auto bundle = std::make_shared<mc::BufferBundleSurfaces>(std::move(swapper), generator);

    gl_animation.init_gl();

    std::shared_ptr<mc::GraphicRegion> texture_res;

    for(;;)
    {
        /* buffer 0 */
        auto client_buffer = bundle->secure_client_buffer();
        auto region = sw_renderer.get_graphic_region_from_package(client_buffer->ipc_package, size);
        render_solid_pattern(region, 0xFF0000FF);
        client_buffer.reset();

        texture_res = bundle->lock_back_buffer();
        gl_animation.render_gl();
        display->post_update();
        texture_res.reset();
        sleep(1);

        /* buffer 1 */
        client_buffer = bundle->secure_client_buffer();
        region = sw_renderer.get_graphic_region_from_package(client_buffer->ipc_package, size);
        render_solid_pattern(region, 0x0000FFFF);
        client_buffer.reset();

        texture_res = bundle->lock_back_buffer();
        gl_animation.render_gl();
        display->post_update();
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
