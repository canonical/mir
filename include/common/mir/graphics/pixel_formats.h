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

#ifndef MIR_GRAPHICS_PIXEL_FORMATS_H_
#define MIR_GRAPHICS_PIXEL_FORMATS_H_

#include "mir_toolkit/common.h"

namespace mir { namespace graphics {

bool is_valid(MirPixelFormat);

MirPixelFormat from_android_format(unsigned int);
unsigned int to_android_format(MirPixelFormat);

MirPixelFormat from_drm_format(unsigned int);
unsigned int to_drm_format(MirPixelFormat);

unsigned int red_bits(MirPixelFormat);
unsigned int green_bits(MirPixelFormat);
unsigned int blue_bits(MirPixelFormat);
unsigned int alpha_bits(MirPixelFormat);

typedef unsigned int GLenum;
mir::graphics::GLenum gl_teximage_format(MirPixelFormat);
mir::graphics::GLenum gl_teximage_type(MirPixelFormat);

const char* to_string(MirPixelFormat);

} } // namespace mir::graphics

#endif // MIR_GRAPHICS_PIXEL_FORMATS_H_
