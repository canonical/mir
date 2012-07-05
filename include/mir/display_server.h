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
class DisplayServer
{
public:
    DisplayServer() : comp(&scenegraph) {}

    virtual void render(graphics::Display* display)
    {
        comp.render(display);
    }
private:

    //TODO remove implementation details from public view
    surfaces::SurfaceStack scenegraph;
    compositor::Compositor comp;
};
}


#endif /* MIR_DISPLAY_SERVER_H_ */
