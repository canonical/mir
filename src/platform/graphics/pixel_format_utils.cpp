/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/graphics/pixel_format_utils.h"

namespace mg = mir::graphics;

// TODO: We could do much better than this with a simple fixed lookup table
//       or similar (LP: #1236254)

bool mg::contains_alpha(MirPixelFormat format)
{
    return (format == mir_pixel_format_abgr_8888 ||
            format == mir_pixel_format_argb_8888 ||
            format == mir_pixel_format_rgba_5551 ||
            format == mir_pixel_format_rgba_4444);
}

bool mg::valid_pixel_format(MirPixelFormat format)
{
    return (format > mir_pixel_format_invalid &&
            format < mir_pixel_formats);
}

int mg::red_channel_depth(MirPixelFormat format)
{
    switch (format)
    {
    case mir_pixel_format_rgb_565:   return 5;
    case mir_pixel_format_rgba_5551: return 5;
    case mir_pixel_format_rgba_4444: return 4;
    default:                         return valid_pixel_format(format) ? 8 : 0;
    }
}

int mg::blue_channel_depth(MirPixelFormat format)
{
    return red_channel_depth(format);
}

int mg::green_channel_depth(MirPixelFormat format)
{
    switch (format)
    {
    case mir_pixel_format_rgb_565:   return 6;
    case mir_pixel_format_rgba_5551: return 5;
    case mir_pixel_format_rgba_4444: return 4;
    default:                         return valid_pixel_format(format) ? 8 : 0;
    }
}

int mg::alpha_channel_depth(MirPixelFormat format)
{
    switch (format)
    {
    case mir_pixel_format_abgr_8888: return 8;
    case mir_pixel_format_argb_8888: return 8;
    case mir_pixel_format_rgba_4444: return 4;
    case mir_pixel_format_rgba_5551: return 1;
    default:                         return 0;
    }
}

