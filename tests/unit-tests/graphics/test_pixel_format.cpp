/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir_toolkit/common.h"
#include "mir/graphics/pixel_format.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using mir::graphics::PixelFormat;
TEST(PixelFormat, contains_alpha)
{
    EXPECT_FALSE(PixelFormat{mir_pixel_format_xbgr_8888}.contains_alpha());
    EXPECT_FALSE(PixelFormat{mir_pixel_format_bgr_888}.contains_alpha());
    EXPECT_FALSE(PixelFormat{mir_pixel_format_xrgb_8888}.contains_alpha());
    EXPECT_FALSE(PixelFormat{mir_pixel_format_xbgr_8888}.contains_alpha());
    EXPECT_TRUE(PixelFormat{mir_pixel_format_argb_8888}.contains_alpha());
    EXPECT_TRUE(PixelFormat{mir_pixel_format_abgr_8888}.contains_alpha());
    EXPECT_FALSE(PixelFormat{mir_pixel_format_invalid}.contains_alpha());
    EXPECT_FALSE(PixelFormat{mir_pixel_formats}.contains_alpha());
}

TEST(PixelFormat, red_channel_depths)
{
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xbgr_8888}.red_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_bgr_888}.red_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xrgb_8888}.red_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xbgr_8888}.red_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_argb_8888}.red_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_abgr_8888}.red_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_invalid}.red_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_formats}.red_channel_depth());
}

TEST(PixelFormat, blue_channel_depths)
{
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xbgr_8888}.blue_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_bgr_888}.blue_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xrgb_8888}.blue_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xbgr_8888}.blue_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_argb_8888}.blue_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_abgr_8888}.blue_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_invalid}.blue_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_formats}.blue_channel_depth());
}

TEST(PixelFormat, green_channel_depths)
{
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xbgr_8888}.green_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_bgr_888}.green_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xrgb_8888}.green_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_xbgr_8888}.green_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_argb_8888}.green_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_abgr_8888}.green_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_invalid}.green_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_formats}.green_channel_depth());
}


TEST(PixelFormat, alpha_channel_depths)
{
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_xbgr_8888}.alpha_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_bgr_888}.alpha_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_xrgb_8888}.alpha_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_xbgr_8888}.alpha_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_argb_8888}.alpha_channel_depth());
    EXPECT_EQ(8, PixelFormat{mir_pixel_format_abgr_8888}.alpha_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_format_invalid}.alpha_channel_depth());
    EXPECT_EQ(0, PixelFormat{mir_pixel_formats}.alpha_channel_depth());
}

TEST(PixelFormat, converts_to_bool)
{
    EXPECT_TRUE(bool(PixelFormat{mir_pixel_format_xbgr_8888}));
    EXPECT_TRUE(bool(PixelFormat{mir_pixel_format_bgr_888}));
    EXPECT_TRUE(bool(PixelFormat{mir_pixel_format_xrgb_8888}));
    EXPECT_TRUE(bool(PixelFormat{mir_pixel_format_xbgr_8888}));
    EXPECT_TRUE(bool(PixelFormat{mir_pixel_format_argb_8888}));
    EXPECT_TRUE(bool(PixelFormat{mir_pixel_format_abgr_8888}));
    EXPECT_FALSE(bool(PixelFormat{mir_pixel_format_invalid}));
    EXPECT_FALSE(bool(PixelFormat{mir_pixel_formats}));
}

TEST(PixelFormat, converts_to_pixel_format)
{
    EXPECT_EQ(mir_pixel_format_argb_8888, PixelFormat{mir_pixel_format_argb_8888});
}
