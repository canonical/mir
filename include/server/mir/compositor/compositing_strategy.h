/*
 * Copyright © 2012-2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_COMPOSITING_STRATEGY_H_
#define MIR_COMPOSITOR_COMPOSITING_STRATEGY_H_

namespace mir
{
namespace graphics
{
class DisplayBuffer;
}
namespace geometry
{
struct Rectangle;
}

namespace compositor
{

class CompositingStrategy
{
public:
    virtual ~CompositingStrategy() = default;

    virtual void render(graphics::DisplayBuffer& display_buffer) = 0;

protected:
    CompositingStrategy() = default;
    CompositingStrategy& operator=(CompositingStrategy const&) = delete;
    CompositingStrategy(CompositingStrategy const&) = delete;
};
}
}


#endif /* MIR_COMPOSITOR_COMPOSITING_STRATEGY_H_ */
