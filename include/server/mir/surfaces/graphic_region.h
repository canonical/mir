/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SURFACES_GRAPHIC_REGION_H_
#define MIR_SURFACES_GRAPHIC_REGION_H_

#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"
#include <memory>

namespace mir
{
namespace surfaces
{

class GraphicRegion
{
public:
    virtual ~GraphicRegion() {}
    virtual geometry::Size size() const = 0;
    virtual geometry::Stride stride() const = 0;
    virtual geometry::PixelFormat pixel_format() const = 0;
    virtual void bind_to_texture() = 0;

protected:
    GraphicRegion() = default;
    GraphicRegion(GraphicRegion const&) = delete;
    GraphicRegion& operator=(GraphicRegion const&) = delete;
};

}
}

#endif /* MIR_SURFACES_GRAPHIC_REGION_H_ */
