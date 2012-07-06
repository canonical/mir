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


#ifndef MIR_DISPLAY_SERVER_H_
#define MIR_DISPLAY_SERVER_H_

#include "mir/compositor/compositor.h"
#include "mir/surfaces/surface_stack.h"

namespace mir
{
namespace compositor
{

class BufferBundleFactory;

}

class DisplayServer
{
public:
    // TODO: Come up with a better way to resolve dependency on
    // the BufferAllocationStrategy.
    DisplayServer(compositor::BufferAllocationStrategy* strategy)
            : buffer_bundle_manager(strategy),
              surface_stack(&buffer_bundle_manager),
              compositor(&surface_stack)              
    {}

    virtual void render(graphics::Display* display)
    {
        compositor.render(display);
    }

private:
    //TODO remove implementation details from public view
    compositor::BufferBundleManager buffer_bundle_manager;
    surfaces::SurfaceStack surface_stack;
    compositor::Compositor compositor;
};
}

#endif /* MIR_DISPLAY_SERVER_H_ */
