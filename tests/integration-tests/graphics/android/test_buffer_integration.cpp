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

#include "src/platform/graphics/android/android_graphic_buffer_allocator.h"
#include "src/platform/graphics/android/buffer_writer.h"
#include "src/server/compositor/buffer_queue.h"
#include "src/server/report/null_report_factory.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir_test_doubles/stub_frame_dropping_policy_factory.h"

#include <hardware/gralloc.h>
#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;

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
    }

    geom::Size size;
    MirPixelFormat pf;
    mg::BufferProperties buffer_properties;
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

    mga::BufferWriter writer;
    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>();

    mg::BufferProperties sw_properties{size, pf, mg::BufferUsage::software};
    auto test_buffer = allocator->alloc_buffer(sw_properties);

    writer.write(*test_buffer, nullptr, 0);

    auto native_buffer = test_buffer->native_buffer_handle();
    native_buffer->ensure_available_for(mga::BufferAccess::write);

    //read using native interface
    gralloc_module_t* module;
    alloc_device_t* alloc_dev;
    const hw_module_t *hw_module;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module) != 0)
        throw std::runtime_error("error, hw module not available!\n");
    gralloc_open(hw_module, &alloc_dev);
    module = (gralloc_module_t*) hw_module;
    int *vaddr;
    module->lock(
        module, native_buffer->handle(), GRALLOC_USAGE_SW_READ_OFTEN,
        0, 0, native_buffer->anwb()->width, native_buffer->anwb()->height,
        (void**) &vaddr);


//    module->unlock();
#if 0 //test should read red back
    for(auto i = 0; i < region.height; i++)
    {
        for(auto j = 0; j < region.width; j++)
        {
            uint32_t *pixel = (uint32_t*) &region.vaddr[i*region.stride + (j * bpp)];
            if (*pixel != color_value)
            {
                return false;
            }
        }
    }
#endif
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
