/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_COMPOSITOR_COMPOSITOR_H_
#define MIR_COMPOSITOR_COMPOSITOR_H_

#include "mir/compositor/drawer.h"

#include <memory>

namespace mir
{
namespace surfaces
{
// scenegraph is the interface compositor uses onto the surface stack
class Scenegraph;
class SurfaceRenderer;
}

namespace compositor
{
class Compositor : public Drawer
{
public:
    explicit Compositor(
        surfaces::Scenegraph* scenegraph,
        const std::shared_ptr<surfaces::SurfaceRenderer>& renderer);

    virtual void render(graphics::Display* display);

private:
    surfaces::Scenegraph* const scenegraph;
    std::shared_ptr<surfaces::SurfaceRenderer> renderer;
};


}
}

#endif /* MIR_COMPOSITOR_COMPOSITOR_H_ */
