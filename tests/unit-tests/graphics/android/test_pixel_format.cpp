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

#include "src/platform/graphics/android/android_format_conversion-inl.h"

#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

TEST(PixelFormatConversion, conversion_to_android_test)
{
    EXPECT_EQ(HAL_PIXEL_FORMAT_RGBA_8888, mga::to_android_format(geom::PixelFormat::abgr_8888));
    EXPECT_EQ(HAL_PIXEL_FORMAT_RGBX_8888, mga::to_android_format(geom::PixelFormat::xbgr_8888));
    EXPECT_EQ(HAL_PIXEL_FORMAT_BGRA_8888, mga::to_android_format(geom::PixelFormat::argb_8888));
    //note X to A conversion!
    EXPECT_EQ(HAL_PIXEL_FORMAT_BGRA_8888, mga::to_android_format(geom::PixelFormat::xrgb_8888));
    EXPECT_EQ(HAL_PIXEL_FORMAT_RGB_888, mga::to_android_format(geom::PixelFormat::bgr_888));
}

TEST(PixelFormatConversion, conversion_to_mir_test)
{
    EXPECT_EQ(geom::PixelFormat::abgr_8888, mga::to_mir_format(HAL_PIXEL_FORMAT_RGBA_8888));
    EXPECT_EQ(geom::PixelFormat::xbgr_8888, mga::to_mir_format(HAL_PIXEL_FORMAT_RGBX_8888));
    EXPECT_EQ(geom::PixelFormat::argb_8888, mga::to_mir_format(HAL_PIXEL_FORMAT_BGRA_8888));
    EXPECT_EQ(geom::PixelFormat::bgr_888, mga::to_mir_format(HAL_PIXEL_FORMAT_RGB_888));
}
