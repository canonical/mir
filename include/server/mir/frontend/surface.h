/*
 * Copyright © 2013 Canonical Ltd.
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
 */


#ifndef MIR_FRONTEND_SURFACE_H_
#define MIR_FRONTEND_SURFACE_H_

#include "mir/frontend/buffer_stream.h"
#include "mir/geometry/size.h"
#include "mir/geometry/displacement.h"

#include "mir_toolkit/common.h"

#include <list>
#include <string>
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
class CursorImage;
}
namespace compositor
{
class BufferStream;
}
namespace frontend
{
class ClientBufferTracker;
class BufferStream;

class Surface
{
public:
    virtual ~Surface() = default;

    /// Size of the client area of the surface (excluding any decorations)
    virtual geometry::Size client_size() const = 0;
    virtual MirPixelFormat pixel_format() const = 0;

    virtual void add_observer(std::shared_ptr<scene::SurfaceObserver> const& observer) = 0;
    virtual void remove_observer(std::weak_ptr<scene::SurfaceObserver> const& observer) = 0;
    
    virtual std::shared_ptr<frontend::BufferStream> primary_buffer_stream() const = 0;

    //insert a new stream at the top-most z-order
    virtual void add_stream(
        std::shared_ptr<compositor::BufferStream> const& stream,
        geometry::Displacement position,
        float alpha) = 0;
    virtual void reposition(compositor::BufferStream const*, geometry::Displacement pt, float alpha) = 0;
    //NOTE: one cannot remove a stream that was never added via add_stream (eg, the primary stream)
    virtual void remove_stream(compositor::BufferStream const*) = 0;
    //raise a bufferstream to the top-most z-order 
    virtual void raise(compositor::BufferStream const*) = 0;

    virtual bool supports_input() const = 0;
    virtual int client_input_fd() const = 0;

    virtual void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) = 0;
    virtual void set_cursor_stream(std::shared_ptr<frontend::BufferStream> const& image,
        geometry::Displacement const& hotspot) = 0;

protected:
    Surface() = default;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};

}
}


#endif /* MIR_FRONTEND_SURFACE_H_ */
