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

#ifndef COMPOSITOR_H_
#define COMPOSITOR_H_

#include "drawer.h"

namespace mir
{
namespace surfaces
{
// scenegraph is the interface compositor uses onto the surface stack
class scenegraph;
}

namespace compositor
{
class buffer_texture_binder;

class compositor : public drawer
{
public:
    explicit compositor(
        surfaces::scenegraph* scenegraph,
        buffer_texture_binder* buffermanager);

    virtual void render(graphics::display* display);

private:
    surfaces::scenegraph* const scenegraph;
    buffer_texture_binder* const buffermanager;
};


}
}

#endif /* COMPOSITOR_H_ */
