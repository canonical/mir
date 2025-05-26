/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_SURFACE_H_
#define MIR_FRONTEND_SURFACE_H_

#include "mir/frontend/buffer_stream.h"
#include "mir/geometry/size.h"
#include "mir/geometry/displacement.h"

#include "mir_toolkit/common.h"

#include <string>
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
class CursorImage;
}
namespace wayland
{
template<typename T>
class Weak;
}
namespace frontend
{
class BufferStream;
class WlSurface;

class Surface
{
public:
    virtual ~Surface() = default;

    /// The offset of the content rect relative to the window's position
    virtual auto content_offset() const -> geometry::Displacement = 0;

    /// Size of the client area of the surface (excluding any decorations)
    virtual auto content_size() const -> geometry::Size = 0;

    virtual void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) = 0;

    /// Returned value is only safe to use on the Wayland thread
    virtual auto wayland_surface() -> wayland::Weak<WlSurface> const& = 0;

protected:
    Surface() = default;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};

}
}


#endif /* MIR_FRONTEND_SURFACE_H_ */
