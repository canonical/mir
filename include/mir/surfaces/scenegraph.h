/*
 * Copyright © 2012 Canonical Ltd.
 *
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

#ifndef MIR_SURFACES_SCENEGRAPH_H_
#define MIR_SURFACES_SCENEGRAPH_H_

#include "mir/geometry/forward.h"
#include "mir/compositor/render_view.h"

#include <memory>

namespace mc = mir::compositor;

namespace mir
{
namespace surfaces
{

class Surface;

// scenegraph is the interface compositor uses onto the surface stack
class Scenegraph : public mc::RenderView
{
public:

    virtual ~Scenegraph() {}

protected:
    Scenegraph() = default;

private:
    Scenegraph(Scenegraph const&) = delete;
    Scenegraph& operator=(Scenegraph const&) = delete;
};
}
}

#endif /* MIR_SURFACES_SCENEGRAPH_H_ */
