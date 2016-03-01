/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/platforms/android/server/framebuffers.h"
#include "src/platforms/android/server/graphic_buffer_allocator.h"
#include "src/platforms/android/server/cmdstream_sync_factory.h"
#include "src/platforms/android/server/device_quirks.h"
#include "mir/test/doubles/mock_android_hw.h"
#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/mock_egl.h"

#include <future>
#include <initializer_list>
#include <thread>
#include <stdexcept>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;
using namespace testing;

namespace
{
struct Framebuffers : Test
{
    NiceMock<mtd::MockEGL> mock_egl;
    NiceMock<mtd::HardwareAccessMock> hw_access_mock;
    mga::GraphicBufferAllocator allocator{
        std::make_shared<mga::NullCommandStreamSyncFactory>(),
        std::make_shared<mga::DeviceQuirks>(mga::PropertiesOps{})};
    MirPixelFormat format{mir_pixel_format_abgr_8888};
    geom::Size display_size{180, 222};
};
}

TEST_F(Framebuffers, framebuffers_have_correct_size)
{
    mga::Framebuffers framebuffers(allocator, display_size, format, 2u);
    EXPECT_EQ(display_size, framebuffers.fb_size());
}

TEST_F(Framebuffers, last_rendered_returns_valid)
{
    mga::Framebuffers framebuffers(allocator, display_size, format, 2u);

    auto first_buffer = framebuffers.buffer_for_render();
    auto first_buffer_ptr = first_buffer.get();
    EXPECT_NE(first_buffer, framebuffers.last_rendered_buffer());
    first_buffer.reset();
    EXPECT_EQ(first_buffer_ptr, framebuffers.last_rendered_buffer().get());
}

TEST_F(Framebuffers, last_rendered_is_first_returned_from_driver)
{
    mga::Framebuffers framebuffers(allocator, display_size, format, 2u);
    auto buffer1 = framebuffers.buffer_for_render().get();
    EXPECT_EQ(buffer1, framebuffers.last_rendered_buffer().get());
    auto buffer2 = framebuffers.buffer_for_render().get();
    EXPECT_EQ(buffer2, framebuffers.last_rendered_buffer().get());
}

TEST_F(Framebuffers, no_rendering_returns_same_buffer)
{
    mga::Framebuffers framebuffers(allocator, display_size, format, 2u);
    framebuffers.buffer_for_render().get();
    auto buffer = framebuffers.last_rendered_buffer();
    EXPECT_EQ(buffer, framebuffers.last_rendered_buffer());
}

TEST_F(Framebuffers, three_buffers_for_hwc)
{
    mga::Framebuffers framebuffers(allocator, display_size, format, 3u);

    auto buffer1 = framebuffers.buffer_for_render().get();
    framebuffers.buffer_for_render();
    framebuffers.buffer_for_render();
    auto buffer4 = framebuffers.buffer_for_render().get();

    EXPECT_EQ(buffer1, buffer4);
}
