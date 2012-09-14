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

#include "mir/graphics/platform.h"
#include "mir/graphics/android/android_buffer_allocator.h"
#include "mir/graphics/android/android_display.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir_test/test_utils_android_graphics.h"

#include "../tools/mir_image.h"
#include <GLES2/gl2.h>
#include <stdexcept>
#include <gtest/gtest.h>
#include <system/window.h>
#include <hardware/gralloc.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry; 
namespace mga=mir::graphics::android; 
namespace mg=mir::graphics; 
namespace mt=mir::test;

class AndroidBufferIntegration : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        allocator = std::make_shared<mga::AndroidBufferAllocator>();
        strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);
        w = geom::Width(200);
        h = geom::Height(400);
        pf  = mc::PixelFormat::rgba_8888;
    }
 
    std::shared_ptr<mga::AndroidBufferAllocator> allocator;
    std::shared_ptr<mc::DoubleBufferAllocationStrategy> strategy;
    geom::Width  w;
    geom::Height h;
    mc::PixelFormat pf;
};

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
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf); 
    });
}

TEST_F(AndroidBufferIntegration, swapper_returns_non_null)
{
    using namespace testing;

    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf);

    EXPECT_NE((int)swapper->client_acquire(), NULL);
}

TEST_F(AndroidBufferIntegration, buffer_throws_with_no_egl_context)
{
    using namespace testing;

    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf);

    auto buffer = swapper->compositor_acquire();
    EXPECT_THROW({
    buffer->bind_to_texture();
    }, std::runtime_error);

}

TEST_F(AndroidBufferIntegration, buffer_ok_with_egl_context_repeat)
{
    using namespace testing;
    std::shared_ptr<mg::Display> display;
    display = mg::create_display();
    mt::glAnimationBasic gl_animation;
    mt::grallocRenderSW sw_renderer;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);


    geom::Width  w(64);
    geom::Height h(64);
    mc::PixelFormat pf(mc::PixelFormat::rgba_8888);
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf);
    auto bundle = std::make_shared<mc::BufferBundle>(std::move(swapper));

    gl_animation.init_gl();

    std::shared_ptr<mg::Texture> texture_res;

#define REPEAT 0
#if REPEAT
    for(;;)
    {
#endif
        /* buffer 0 */
        auto client_buffer = bundle->secure_client_buffer();
        sw_renderer.render_pattern(client_buffer, 0xFF0000FF);
        client_buffer.reset();

        texture_res = bundle->lock_and_bind_back_buffer();
        gl_animation.render_gl();
        display->post_update();
        texture_res.reset();
#if REPEAT
        sleep(1);

        /* buffer 1 */
        client_buffer = bundle->secure_client_buffer();
        sw_renderer.render_pattern(client_buffer, 0x0000FFFF);
        client_buffer.reset();

        texture_res = bundle->lock_and_bind_back_buffer();
        gl_animation.render_gl();
        display->post_update();
        texture_res.reset();
        sleep(1);
    }
#endif

}

