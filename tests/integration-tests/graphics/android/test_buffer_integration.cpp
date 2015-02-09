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

#include "src/platforms/android/server/android_graphic_buffer_allocator.h"
#include "src/server/compositor/buffer_queue.h"
#include "src/server/report/null_report_factory.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "graphics_region_factory.h"
#include "patterns.h"

#include "mir_test_doubles/stub_frame_dropping_policy_factory.h"
#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace mt=mir::test;

namespace
{
class AndroidBufferIntegration : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        size = geom::Size{334, 122};
        pf  = mir_pixel_format_abgr_8888;
        buffer_properties = mg::BufferProperties{size, pf, mg::BufferUsage::software};
        graphics_region_factory = std::make_shared<mt::GraphicsRegionFactory>();
    }

    geom::Size size;
    MirPixelFormat pf;
    mg::BufferProperties buffer_properties;
    std::shared_ptr<mt::GraphicsRegionFactory> graphics_region_factory;
    mir::test::doubles::StubFrameDroppingPolicyFactory policy_factory;
};

auto client_acquire_blocking(mc::BufferQueue& switching_bundle)
-> mg::Buffer*
{
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;

    mg::Buffer* result;
    switching_bundle.client_acquire(
        [&](mg::Buffer* new_buffer)
         {
            std::unique_lock<decltype(mutex)> lock(mutex);

            result = new_buffer;
            done = true;
            cv.notify_one();
         });

    std::unique_lock<decltype(mutex)> lock(mutex);

    cv.wait(lock, [&]{ return done; });

    return result;
}
}

TEST_F(AndroidBufferIntegration, allocator_can_create_sw_buffer)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>();

    mg::BufferProperties sw_properties{size, pf, mg::BufferUsage::software};
    auto test_buffer = allocator->alloc_buffer(sw_properties);

    auto region = graphics_region_factory->graphic_region_from_handle(
        *test_buffer->native_buffer_handle());
    mt::DrawPatternSolid red_pattern(0xFF0000FF);
    red_pattern.draw(*region);
    EXPECT_TRUE(red_pattern.check(*region));
}

TEST_F(AndroidBufferIntegration, allocator_can_create_hw_buffer)
{
    using namespace testing;

    mg::BufferProperties hw_properties{size, pf, mg::BufferUsage::hardware};
    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>();

    //TODO: kdub it is a bit trickier to test that a gpu can render... just check creation for now
    auto test_buffer = allocator->alloc_buffer(hw_properties);
    EXPECT_NE(nullptr, test_buffer);
}

TEST_F(AndroidBufferIntegration, swapper_creation_is_sane)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>();

    mc::BufferQueue swapper(2, allocator, buffer_properties, policy_factory);

    auto returned_buffer = client_acquire_blocking(swapper);

    EXPECT_NE(nullptr, returned_buffer);
}
