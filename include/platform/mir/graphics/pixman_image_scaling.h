/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_PIXMAN_IMAGE_SCALING_H_
#define MIR_GRAPHICS_PIXMAN_IMAGE_SCALING_H_

#include "mir/geometry/size.h"
#include <memory>

namespace mir::graphics
{
struct PixBuffer
{
    std::unique_ptr<uint32_t[]> data;
    geometry::Size size;
};

class CursorImage;
PixBuffer scale_cursor_image(std::shared_ptr<CursorImage> const& cursor_image, float new_scale);
}

#endif
