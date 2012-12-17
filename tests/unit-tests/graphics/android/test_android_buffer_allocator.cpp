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

#include "src/graphics/android/android_buffer_allocator.h"

#include <algorithm>

#include <gtest/gtest.h>

namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

class AndroidBufferAllocatorTest : public ::testing::Test
{
public:
    mga::AndroidBufferAllocator allocator;
};

TEST_F(AndroidBufferAllocatorTest, supported_pixel_formats_contain_common_formats)
{
    auto supported_pixel_formats = allocator.supported_pixel_formats();

    auto rgba_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      geom::PixelFormat::rgba_8888);

    auto rgbx_8888_count = std::count(supported_pixel_formats.begin(),
                                      supported_pixel_formats.end(),
                                      geom::PixelFormat::rgbx_8888);

    auto rgb_888_count = std::count(supported_pixel_formats.begin(),
                                    supported_pixel_formats.end(),
                                    geom::PixelFormat::rgb_888);

    EXPECT_EQ(1, rgba_8888_count);
    EXPECT_EQ(1, rgbx_8888_count);
    EXPECT_EQ(1, rgb_888_count);
}

TEST_F(AndroidBufferAllocatorTest, supported_pixel_formats_have_sane_default_in_first_position)
{
    auto supported_pixel_formats = allocator.supported_pixel_formats();

    ASSERT_FALSE(supported_pixel_formats.empty());
    EXPECT_EQ(geom::PixelFormat::rgba_8888, supported_pixel_formats[0]);
}
