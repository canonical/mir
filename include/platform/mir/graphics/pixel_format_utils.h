/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_PIXEL_FORMAT_UTILS_H_
#define MIR_GRAPHICS_PIXEL_FORMAT_UTILS_H_

#include <mir_toolkit/common.h>

namespace mir
{
namespace graphics
{

/*!
 * \name MirPixelFormat utility functions
 *
 * A set of functions to query details of MirPixelFormat
 * TODO improve this through https://bugs.launchpad.net/mir/+bug/1236254
 * \{
 */
bool contains_alpha(MirPixelFormat format);
int red_channel_depth(MirPixelFormat format);
int blue_channel_depth(MirPixelFormat format);
int green_channel_depth(MirPixelFormat format);
int alpha_channel_depth(MirPixelFormat format);
bool valid_pixel_format(MirPixelFormat format);
/*!
 * \}
 */


}
}

#endif
