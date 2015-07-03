/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/pixel_formats.h"
#include "mir_toolkit/common.h"

// Use the headers but don't link to the libraries:
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace {

typedef struct
{
    MirPixelFormat mir_format;
    unsigned int bytes_per_pixel;
    unsigned int red_bits, green_bits, blue_bits, alpha_bits;
    const char * const name;
    GLenum gl_le_format, gl_type;  // Parameters for glTexImage2D
} Detail;

static const Detail detail[mir_pixel_formats] =
{
    {mir_pixel_format_invalid,   0, 0,0,0,0, "invalid",
         GL_INVALID_ENUM, GL_INVALID_ENUM},
    {mir_pixel_format_abgr_8888, 4, 8,8,8,8, "abgr_8888",
         GL_RGBA, GL_UNSIGNED_BYTE},
    {mir_pixel_format_xbgr_8888, 4, 8,8,8,8, "xbgr_8888",
         GL_RGBA, GL_UNSIGNED_BYTE},
    {mir_pixel_format_argb_8888, 4, 8,8,8,8, "argb_8888",
         GL_BGRA_EXT, GL_UNSIGNED_BYTE},
    {mir_pixel_format_xrgb_8888, 4, 8,8,8,8, "xrgb_8888",
         GL_BGRA_EXT, GL_UNSIGNED_BYTE},
    {mir_pixel_format_bgr_888,   3, 8,8,8,0, "bgr_8888",
         GL_INVALID_ENUM, GL_INVALID_ENUM},  /* No OpenGL support! */
    {mir_pixel_format_invalid /* TODO */, 3, 8,8,8,8, "rgb_8888",
         GL_RGB, GL_UNSIGNED_BYTE},
    {mir_pixel_format_rgb_565,   2, 5,6,5,0, "rgb_565",
         GL_RGB, GL_UNSIGNED_SHORT_5_6_5},
    {mir_pixel_format_rgba_5551, 2, 5,5,5,1, "rgba_5551",
         GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1},
    {mir_pixel_format_rgba_4444, 2, 4,4,4,4, "rgba_4444",
         GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4},
};

} // anonymous namespace

namespace mir { namespace graphics {

bool is_valid(MirPixelFormat f)
{
    return (f > mir_pixel_format_invalid) &&
           (f < mir_pixel_formats) &&
           (detail[f].mir_format == f);
}

unsigned int bytes_per_pixel(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].bytes_per_pixel : 0;
}

unsigned int red_bits(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].red_bits : 0;
}

unsigned int green_bits(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].green_bits : 0;
}

unsigned int blue_bits(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].blue_bits : 0;
}

unsigned int alpha_bits(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].alpha_bits : 0;
}

GLenum gl_teximage_format(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].gl_le_format : GL_INVALID_ENUM;
}

GLenum gl_teximage_type(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].gl_type : GL_INVALID_ENUM;
}

const char* to_string(MirPixelFormat f)
{
    return is_valid(f) ? detail[f].name : "unknown";
}

} } // namespace mir::graphics
