/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MIR_GRAPHICS_CURSOR_IMAGE_H_
#define MIR_GRAPHICS_CURSOR_IMAGE_H_

#include <mir/geometry/size.h>
#include <mir/geometry/displacement.h>

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;

class CursorImage
{
public:
    virtual ~CursorImage() = default;

    virtual void const* as_argb_8888() const = 0;
    virtual geometry::Size size() const = 0;

    // We use "hotspot" to mean the offset within a cursor image
    // which should be placed at the onscreen
    // location of the pointer.
    virtual geometry::Displacement hotspot() const = 0;

    /// Returns the underlying graphics buffer if available.
    ///
    /// Some cursor images may be backed by a hardware buffer that cannot be
    /// CPU-mapped. In this case, as_argb_8888() will return nullptr and the
    /// buffer should be used for rendering directly.
    ///
    /// \returns The underlying buffer, or nullptr if the cursor uses CPU pixel data.
    auto buffer() const -> std::shared_ptr<Buffer> { return nullptr; }

protected:
    CursorImage() = default;
    CursorImage(CursorImage const&) = delete;
    CursorImage& operator=(CursorImage const&) = delete;
};
}
}


#endif /* MIR_GRAPHICS_CURSOR_IMAGE_H_ */
