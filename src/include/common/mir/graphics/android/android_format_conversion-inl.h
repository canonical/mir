/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_FORMAT_CONVERSION_INL_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_FORMAT_CONVERSION_INL_H_

#include "mir_toolkit/common.h"
#include <system/graphics.h>

namespace mir
{
namespace graphics
{
namespace android
{

inline static int to_android_format(MirPixelFormat format)
{
    switch(format)
    {
        case mir_pixel_format_abgr_8888:
            return HAL_PIXEL_FORMAT_RGBA_8888;
        case mir_pixel_format_xbgr_8888:
            return HAL_PIXEL_FORMAT_RGBX_8888;
        case mir_pixel_format_argb_8888:
            return HAL_PIXEL_FORMAT_BGRA_8888;
        case mir_pixel_format_xrgb_8888:
            return HAL_PIXEL_FORMAT_BGRA_8888;
        case mir_pixel_format_rgb_888:
            return HAL_PIXEL_FORMAT_RGB_888;
        case mir_pixel_format_rgb_565:
            return HAL_PIXEL_FORMAT_RGB_565;
        default:
            return 0;
    }
}

inline static MirPixelFormat to_mir_format(int format)
{
    switch(format)
    {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return mir_pixel_format_abgr_8888;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return mir_pixel_format_xbgr_8888;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            return mir_pixel_format_argb_8888;
        case HAL_PIXEL_FORMAT_RGB_888:
            return mir_pixel_format_rgb_888;
        case HAL_PIXEL_FORMAT_RGB_565:
            return mir_pixel_format_rgb_565;
        default:
            return mir_pixel_format_invalid;
    }
}

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_FORMAT_CONVERSION_INL_H_ */
