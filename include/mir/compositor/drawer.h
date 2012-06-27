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

#ifndef MIR_COMPOSITOR_DRAWER_H_
#define MIR_COMPOSITOR_DRAWER_H_

namespace mir
{
namespace graphics
{
class Display;
}

namespace compositor
{

// drawer is the interface by which "graphics/libgl" knows
// the compositor.
class drawer
{
public:
    virtual void render(graphics::Display* display) = 0;
protected:
    drawer() = default;
    ~drawer() = default;
private:
    drawer& operator=(drawer const&) = delete;
    drawer(drawer const&) = delete;
};
}
}


#endif /* MIR_COMPOSITOR_DRAWER_H_ */
