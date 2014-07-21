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

#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
class InternalSurface;
class CursorImage;
}

namespace frontend
{
class ClientBufferTracker;

class Surface
{
public:
    virtual ~Surface() = default;

    /// Size of the client area of the surface (excluding any decorations)
    virtual geometry::Size client_size() const = 0;
    virtual MirPixelFormat pixel_format() const = 0;

    virtual void swap_buffers(graphics::Buffer* old_buffer, std::function<void(graphics::Buffer* new_buffer)> complete) = 0;

    virtual bool supports_input() const = 0;
    virtual int client_input_fd() const = 0;

    virtual int configure(MirSurfaceAttrib attrib, int value) = 0;
    virtual int query(MirSurfaceAttrib attrib) = 0;

    virtual void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) = 0;

    /**
     *  swap_buffers_blocking() is a convenience wrapper around swap_buffers()
     *  it forces the current thread to block until complete() is called.
     *  Use with care!
     */
    void swap_buffers_blocking(graphics::Buffer*& buffer);

protected:
    Surface() = default;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};

auto as_internal_surface(std::shared_ptr<Surface> const& surface)
    -> std::shared_ptr<graphics::InternalSurface>;
}
}


#endif /* MIR_FRONTEND_SURFACE_H_ */
