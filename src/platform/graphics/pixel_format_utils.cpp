/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/pixel_format_utils.h"

namespace {

const struct
{
    MirPixelFormat mir_format;
    int red_bits, green_bits, blue_bits, alpha_bits;
} detail[mir_pixel_formats] =
{
    {mir_pixel_format_invalid,   0,0,0,0},
    {mir_pixel_format_abgr_8888, 8,8,8,8},
    {mir_pixel_format_xbgr_8888, 8,8,8,0},
    {mir_pixel_format_argb_8888, 8,8,8,8},
    {mir_pixel_format_xrgb_8888, 8,8,8,0},
    {mir_pixel_format_bgr_888,   8,8,8,0},
    {mir_pixel_format_rgb_888,   8,8,8,0},
    {mir_pixel_format_rgb_565,   5,6,5,0},
    {mir_pixel_format_rgba_5551, 5,5,5,1},
    {mir_pixel_format_rgba_4444, 4,4,4,4},
};

} // anonymous namespace

namespace mir { namespace graphics {

bool valid_pixel_format(MirPixelFormat f)
{
    return f > mir_pixel_format_invalid &&
           f < mir_pixel_formats &&
           detail[f].mir_format == f;
}

int red_channel_depth(MirPixelFormat f)
{
    return valid_pixel_format(f) ? detail[f].red_bits : 0;
}

int green_channel_depth(MirPixelFormat f)
{
    return valid_pixel_format(f) ? detail[f].green_bits : 0;
}

int blue_channel_depth(MirPixelFormat f)
{
    return valid_pixel_format(f) ? detail[f].blue_bits : 0;
}

int alpha_channel_depth(MirPixelFormat f)
{
    return valid_pixel_format(f) ? detail[f].alpha_bits : 0;
}

bool contains_alpha(MirPixelFormat format)
{
    return alpha_channel_depth(format);
}

} } // namespace mir::graphics
