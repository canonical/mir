/*
 * Copyright © 2014 Canonical Ltd.
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
#include "mir/graphics/pixel_format_utils.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace mir::graphics;
TEST(MirPixelFormatUtils, contains_alpha)
{
    EXPECT_FALSE(contains_alpha(mir_pixel_format_xbgr_8888));
    EXPECT_FALSE(contains_alpha(mir_pixel_format_bgr_888));
    EXPECT_FALSE(contains_alpha(mir_pixel_format_rgb_888));
    EXPECT_FALSE(contains_alpha(mir_pixel_format_xrgb_8888));
    EXPECT_FALSE(contains_alpha(mir_pixel_format_xbgr_8888));
    EXPECT_FALSE(contains_alpha(mir_pixel_format_rgb_565));
    EXPECT_TRUE(contains_alpha(mir_pixel_format_argb_8888));
    EXPECT_TRUE(contains_alpha(mir_pixel_format_abgr_8888));
    EXPECT_TRUE(contains_alpha(mir_pixel_format_rgba_5551));
    EXPECT_TRUE(contains_alpha(mir_pixel_format_rgba_4444));
    EXPECT_FALSE(contains_alpha(mir_pixel_format_invalid));
    EXPECT_FALSE(contains_alpha(mir_pixel_formats));
}

TEST(MirPixelFormatUtils, red_channel_depths)
{
    EXPECT_EQ(8, red_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(8, red_channel_depth(mir_pixel_format_bgr_888));
    EXPECT_EQ(8, red_channel_depth(mir_pixel_format_rgb_888));
    EXPECT_EQ(8, red_channel_depth(mir_pixel_format_xrgb_8888));
    EXPECT_EQ(8, red_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(8, red_channel_depth(mir_pixel_format_argb_8888));
    EXPECT_EQ(8, red_channel_depth(mir_pixel_format_abgr_8888));
    EXPECT_EQ(5, red_channel_depth(mir_pixel_format_rgb_565));
    EXPECT_EQ(5, red_channel_depth(mir_pixel_format_rgba_5551));
    EXPECT_EQ(4, red_channel_depth(mir_pixel_format_rgba_4444));
    EXPECT_EQ(0, red_channel_depth(mir_pixel_format_invalid));
    EXPECT_EQ(0, red_channel_depth(mir_pixel_formats));
}

TEST(MirPixelFormatUtils, blue_channel_depths)
{
    EXPECT_EQ(8, blue_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(8, blue_channel_depth(mir_pixel_format_bgr_888));
    EXPECT_EQ(8, blue_channel_depth(mir_pixel_format_rgb_888));
    EXPECT_EQ(8, blue_channel_depth(mir_pixel_format_xrgb_8888));
    EXPECT_EQ(8, blue_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(8, blue_channel_depth(mir_pixel_format_argb_8888));
    EXPECT_EQ(8, blue_channel_depth(mir_pixel_format_abgr_8888));
    EXPECT_EQ(5, blue_channel_depth(mir_pixel_format_rgb_565));
    EXPECT_EQ(5, blue_channel_depth(mir_pixel_format_rgba_5551));
    EXPECT_EQ(4, blue_channel_depth(mir_pixel_format_rgba_4444));
    EXPECT_EQ(0, blue_channel_depth(mir_pixel_format_invalid));
    EXPECT_EQ(0, blue_channel_depth(mir_pixel_formats));
}

TEST(MirPixelFormatUtils, green_channel_depths)
{
    EXPECT_EQ(8, green_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(8, green_channel_depth(mir_pixel_format_bgr_888));
    EXPECT_EQ(8, green_channel_depth(mir_pixel_format_rgb_888));
    EXPECT_EQ(8, green_channel_depth(mir_pixel_format_xrgb_8888));
    EXPECT_EQ(8, green_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(8, green_channel_depth(mir_pixel_format_argb_8888));
    EXPECT_EQ(8, green_channel_depth(mir_pixel_format_abgr_8888));
    EXPECT_EQ(6, green_channel_depth(mir_pixel_format_rgb_565));
    EXPECT_EQ(5, green_channel_depth(mir_pixel_format_rgba_5551));
    EXPECT_EQ(4, green_channel_depth(mir_pixel_format_rgba_4444));
    EXPECT_EQ(0, green_channel_depth(mir_pixel_format_invalid));
    EXPECT_EQ(0, green_channel_depth(mir_pixel_formats));
}


TEST(MirPixelFormatUtils, alpha_channel_depths)
{
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_format_bgr_888));
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_format_rgb_888));
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_format_xrgb_8888));
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_format_xbgr_8888));
    EXPECT_EQ(8, alpha_channel_depth(mir_pixel_format_argb_8888));
    EXPECT_EQ(8, alpha_channel_depth(mir_pixel_format_abgr_8888));
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_format_rgb_565));
    EXPECT_EQ(1, alpha_channel_depth(mir_pixel_format_rgba_5551));
    EXPECT_EQ(4, alpha_channel_depth(mir_pixel_format_rgba_4444));
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_format_invalid));
    EXPECT_EQ(0, alpha_channel_depth(mir_pixel_formats));
}

TEST(MirPixelFormatUtils, valid_mir_pixel_format)
{
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_xbgr_8888));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_bgr_888));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_rgb_888));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_xrgb_8888));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_xbgr_8888));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_argb_8888));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_abgr_8888));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_rgb_565));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_rgba_5551));
    EXPECT_TRUE(valid_pixel_format(mir_pixel_format_rgba_4444));
    EXPECT_FALSE(valid_pixel_format(mir_pixel_format_invalid));
    EXPECT_FALSE(valid_pixel_format(mir_pixel_formats));
}
