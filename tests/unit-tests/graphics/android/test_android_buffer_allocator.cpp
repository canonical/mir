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

#include "src/server/graphics/android/android_buffer_allocator.h"
#include "mir_test/hw_mock.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/buffer_properties.h"

#include "mir_test_doubles/mock_buffer_initializer.h"

#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <algorithm>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mc = mir::compositor;
namespace mtd = mir::test::doubles;

struct AndroidBufferAllocatorTest : public ::testing::Test
{
    AndroidBufferAllocatorTest()
        : null_buffer_initializer{std::make_shared<mg::NullBufferInitializer>()}
    {
    }

    std::shared_ptr<mg::BufferInitializer> const null_buffer_initializer;
    testing::NiceMock<mt::HardwareAccessMock> hw_access_mock;
};

TEST_F(AndroidBufferAllocatorTest, allocator_accesses_gralloc_module)
{
    using namespace testing;

    EXPECT_CALL(hw_access_mock, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID), _))
        .Times(1);

    mga::AndroidBufferAllocator allocator(null_buffer_initializer);
}

TEST_F(AndroidBufferAllocatorTest, supported_pixel_formats_contain_common_formats)
{
    mga::AndroidBufferAllocator allocator{null_buffer_initializer};
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

TEST_F(AndroidBufferAllocatorTest, supported_pixel_formats_have_sane_default_in_first_position)
{
    mga::AndroidBufferAllocator allocator{null_buffer_initializer};
    auto supported_pixel_formats = allocator.supported_pixel_formats();

    ASSERT_FALSE(supported_pixel_formats.empty());
    EXPECT_EQ(geom::PixelFormat::abgr_8888, supported_pixel_formats[0]);
}

TEST_F(AndroidBufferAllocatorTest, alloc_buffer_calls_initializer)
{
    using namespace testing;

    auto buffer_initializer = std::make_shared<mtd::MockBufferInitializer>();

    mga::AndroidBufferAllocator allocator{buffer_initializer};
    mc::BufferProperties properties{geom::Size{geom::Width{2}, geom::Height{2}},
                                    geom::PixelFormat::abgr_8888,
                                    mc::BufferUsage::hardware};

    EXPECT_CALL(*buffer_initializer, operator_call(_))
        .Times(1);

    allocator.alloc_buffer(properties);
}
