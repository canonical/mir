/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SURFACES_SURFACESTACK_H_
#define MIR_SURFACES_SURFACESTACK_H_

#include "scenegraph.h"

#include <memory>

namespace mir
{
namespace surfaces
{

class Surface;

class SurfaceStack : public Scenegraph
{
public:
    virtual SurfacesToRender get_surfaces_in(geometry::Rectangle const& display_area);

    virtual void add_surface(std::weak_ptr<Surface> surface);
    
    virtual void remove_surface(std::weak_ptr<Surface> surface);
};

}
}

#endif /* MIR_SURFACES_SURFACESTACK_H_ */
