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

#include "src/server/graphics/android/android_graphic_buffer_allocator.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/buffer_properties.h"

#include "mir_test_doubles/mock_buffer_initializer.h"
#include "mir_test_doubles/mock_egl.h"

#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <algorithm>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

struct AndroidGraphicBufferAllocatorTest : public ::testing::Test
{
    AndroidGraphicBufferAllocatorTest()
        : null_buffer_initializer{std::make_shared<mg::NullBufferInitializer>()}
    {
    }

    std::shared_ptr<mg::BufferInitializer> const null_buffer_initializer;
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
    mtd::MockEGL mock_egl;
};

TEST_F(AndroidGraphicBufferAllocatorTest, allocator_accesses_gralloc_module)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::AndroidGraphicBufferAllocator allocator(null_buffer_initializer);
}

TEST_F(AndroidGraphicBufferAllocatorTest, supported_pixel_formats_contain_common_formats)
{
    mga::AndroidGraphicBufferAllocator allocator{null_buffer_initializer};
    auto supported_pixel_formats = allocator.supported_pixel_formats();

    auto abgr_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      geom::PixelFormat::abgr_8888);

    auto xbgr_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      geom::PixelFormat::xbgr_8888);

    auto bgr_888_count = std::count(supported_pixel_formats.begin(),
                                    supported_pixel_formats.end(),
                                    geom::PixelFormat::bgr_888);

    EXPECT_EQ(1, abgr_8888_count);
    EXPECT_EQ(1, xbgr_8888_count);
    EXPECT_EQ(1, bgr_888_count);
}

TEST_F(AndroidGraphicBufferAllocatorTest, supported_pixel_formats_have_sane_default_in_first_position)
{
    mga::AndroidGraphicBufferAllocator allocator{null_buffer_initializer};
    auto supported_pixel_formats = allocator.supported_pixel_formats();

    ASSERT_FALSE(supported_pixel_formats.empty());
    EXPECT_EQ(geom::PixelFormat::abgr_8888, supported_pixel_formats[0]);
}

TEST_F(AndroidGraphicBufferAllocatorTest, alloc_buffer_calls_initializer)
{
    using namespace testing;

    auto buffer_initializer = std::make_shared<mtd::MockBufferInitializer>();

    mg::BufferProperties properties{geom::Size{2, 2},
                                    geom::PixelFormat::abgr_8888,
                                    mg::BufferUsage::hardware};
    mga::AndroidGraphicBufferAllocator allocator{buffer_initializer};
    EXPECT_CALL(*buffer_initializer, operator_call(_))
        .Times(1);

    allocator.alloc_buffer(properties);
}

TEST_F(AndroidGraphicBufferAllocatorTest, alloc_buffer_platform_calls_initializer)
{
    using namespace testing;

    auto buffer_initializer = std::make_shared<mtd::MockBufferInitializer>();

    mga::AndroidGraphicBufferAllocator allocator{buffer_initializer};
    auto size = geom::Size{2, 2};
    auto pf = geom::PixelFormat::abgr_8888;

    EXPECT_CALL(*buffer_initializer, operator_call(_))
        .Times(1);

    allocator.alloc_buffer_platform(size, pf, mga::BufferUsage::use_hardware);
}

TEST_F(AndroidGraphicBufferAllocatorTest, buffer_usage_converter)
{
    EXPECT_EQ(mga::BufferUsage::use_hardware,
        mga::AndroidGraphicBufferAllocator::convert_from_compositor_usage(mg::BufferUsage::hardware));
    EXPECT_EQ(mga::BufferUsage::use_software,
        mga::AndroidGraphicBufferAllocator::convert_from_compositor_usage(mg::BufferUsage::software));
}
