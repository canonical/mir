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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */


#ifndef MIR_GRAPHICS_CURSOR_IMAGE_H_
#define MIR_GRAPHICS_CURSOR_IMAGE_H_

#include "mir/geometry/size.h"
#include "mir/geometry/displacement.h"

namespace mir
{
namespace graphics
{
class CursorImage
{
public:
    virtual ~CursorImage() = default;

    virtual void const* as_argb_8888() const = 0;
    virtual geometry::Size size() const = 0;
    
    // We use "hotspot" to mean the offset within a cursor image
    // which should be placed at the onscreen
    // location of the pointer.
    virtual geometry::Displacement hotspot() const = 0;
    

protected:
    CursorImage() = default;
    CursorImage(CursorImage const&) = delete;
    CursorImage& operator=(CursorImage const&) = delete;
};
}
}


#endif /* MIR_GRAPHICS_CURSOR_IMAGE_H_ */
