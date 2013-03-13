/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_SHELL_SURFACE_H_
#define MIR_SHELL_SURFACE_H_

#include "mir/geometry/pixel_format.h"
#include "mir/geometry/point.h"
#include "mir/geometry/size.h"

#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;
}

namespace shell
{

class Surface
{
public:

    virtual ~Surface() {}

    virtual void hide() = 0;
    virtual void show() = 0;
    virtual void destroy() = 0;
    virtual void shutdown() = 0;

    virtual geometry::Size size() const = 0;
    virtual geometry::PixelFormat pixel_format() const = 0;

    // TODO client code always (and necessarily) calls advance_client_buffer()
    // TODO and then client_buffer(). That's a bad interface.
    virtual void advance_client_buffer() = 0;
    virtual std::shared_ptr<compositor::Buffer> client_buffer() const = 0;

protected:
    Surface() = default;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};

}
}


#endif /* MIR_SHELL_SURFACE_H_ */
