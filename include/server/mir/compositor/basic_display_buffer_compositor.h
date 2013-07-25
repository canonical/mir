/*
 * Copyright © 2013 Canonical Ltd.
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


#ifndef MIR_COMPOSITOR_BASIC_DISPLAY_BUFFER_COMPOSITOR_H_
#define MIR_COMPOSITOR_BASIC_DISPLAY_BUFFER_COMPOSITOR_H_

#include "mir/compositor/display_buffer_compositor.h"

#include <functional>
#include <memory>

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
class BasicDisplayBufferCompositor : public DisplayBufferCompositor
{
public:
    BasicDisplayBufferCompositor(graphics::DisplayBuffer& display_buffer);

    virtual void composite();
    virtual void compose(
        mir::geometry::Rectangle const& view_area,
        std::function<void(std::shared_ptr<void> const&)> save_resource) = 0;

protected:
    graphics::DisplayBuffer& display_buffer;
};
}
}

#endif /* MIR_COMPOSITOR_BASIC_DISPLAY_BUFFER_COMPOSITOR_H_ */
