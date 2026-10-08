/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_EXT_FOREIGN_BUFFER_V1_H
#define MIR_FRONTEND_EXT_FOREIGN_BUFFER_V1_H

#include "ext-foreign-buffer-v1_wrapper.h"
#include "wayland_wrapper.h"

#include <mir/wayland/weak.h>

#include <memory>
#include <vector>

namespace mir
{
namespace graphics
{
class Buffer;
class GraphicBufferAllocator;
}
namespace frontend
{
class WlSurface;

/// An opaque wl_buffer handed out by an ext_foreign_buffer_session_v1.
///
/// The requesting client may attach this to one of its own surfaces but never sees the contents:
/// the compositor resolves the handle back to the buffer holding the captured source when the
/// surface is committed. Unlike an ordinary wl_buffer the backing storage is compositor-internal
/// and is expected to change as the source is updated.
///
/// This outlives the session that created it, so it owns the last content it was given rather
/// than reaching back into the session.
class ExtForeignBufferV1 : public wayland::Buffer
{
public:
    explicit ExtForeignBufferV1(wl_resource* resource);

    static auto from(wl_resource* resource) -> ExtForeignBufferV1*;

    /// The buffer currently holding the source's content, or nullptr if nothing has been
    /// captured yet.
    auto content() const -> std::shared_ptr<graphics::Buffer>;

    /// Register a surface to be refreshed whenever the compositor updates the content.
    void add_listener(WlSurface& surface);

    /// Replace the content and push the update to every registered consumer.
    void set_content(std::shared_ptr<graphics::Buffer> buffer);

private:
    std::shared_ptr<graphics::Buffer> current;
    std::vector<wayland::Weak<WlSurface>> consumers;
};

auto create_ext_foreign_buffer_manager_v1(
    wl_display* display,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator)
    -> std::shared_ptr<wayland::ForeignBufferManagerV1::Global>;
}
}

#endif
