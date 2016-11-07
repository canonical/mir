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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platforms/android/server/graphic_buffer_allocator.h"
#include "src/platforms/android/server/device_quirks.h"
#include "src/platforms/android/server/cmdstream_sync_factory.h"
#include "mir/test/doubles/mock_android_hw.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "native_buffer.h"

#include "mir/test/doubles/stub_display_builder.h"
#include "mir/test/doubles/stub_cmdstream_sync_factory.h"
#include "mir/test/doubles/mock_egl.h"

#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <algorithm>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
using namespace testing;

namespace
{
struct GraphicBufferAllocator : Test
{
    NiceMock<mtd::HardwareAccessMock> hw_access_mock;
    NiceMock<mtd::MockEGL> mock_egl;
    mga::GraphicBufferAllocator allocator{
        std::make_shared<mga::NullCommandStreamSyncFactory>(),
        std::make_shared<mga::DeviceQuirks>(mga::PropertiesOps{})};
};
}

TEST_F(GraphicBufferAllocator, allocator_accesses_gralloc_module)
{
    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);

    auto quirks = std::make_shared<mga::DeviceQuirks>(mga::PropertiesOps{});
    mga::GraphicBufferAllocator allocator{std::make_shared<mtd::StubCmdStreamSyncFactory>(), quirks};
}

TEST_F(GraphicBufferAllocator, supported_pixel_formats_contain_common_formats)
{
    auto supported_pixel_formats = allocator.supported_pixel_formats();

    auto abgr_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      mir_pixel_format_abgr_8888);

    auto xbgr_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      mir_pixel_format_xbgr_8888);

    auto bgr_888_count = std::count(supported_pixel_formats.begin(),
                                    supported_pixel_formats.end(),
                                    mir_pixel_format_bgr_888);

    auto rgb_888_count = std::count(supported_pixel_formats.begin(),
                                    supported_pixel_formats.end(),
                                    mir_pixel_format_rgb_888);

    auto rgb_565_count = std::count(supported_pixel_formats.begin(),
                                    supported_pixel_formats.end(),
                                    mir_pixel_format_rgb_565);

    EXPECT_EQ(1, abgr_8888_count);
    EXPECT_EQ(1, xbgr_8888_count);
    EXPECT_EQ(0, bgr_888_count);
    EXPECT_EQ(1, rgb_888_count);
    EXPECT_EQ(1, rgb_565_count);
}

TEST_F(GraphicBufferAllocator, supported_pixel_formats_have_sane_default_in_first_position)
{
    auto supported_pixel_formats = allocator.supported_pixel_formats();

    ASSERT_FALSE(supported_pixel_formats.empty());
    EXPECT_EQ(mir_pixel_format_abgr_8888, supported_pixel_formats[0]);
}
