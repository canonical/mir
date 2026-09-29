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

#include <memory>

namespace mir
{
class Executor;
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
class ExtForeignBufferV1 : public wayland::Buffer
{
public:
    using wayland::Buffer::Buffer;

    static auto from(wl_resource* resource) -> ExtForeignBufferV1*;

    /// The buffer currently holding the source's content, or nullptr if nothing has been
    /// captured yet.
    virtual auto content() const -> std::shared_ptr<graphics::Buffer> = 0;

    /// Register a surface to be refreshed whenever the compositor updates the content.
    ///
    /// The protocol requires the content to stay current once the buffer has been attached and
    /// committed, so the compositor pushes each update to the surfaces displaying it rather than
    /// waiting for the client to attach again. Registering the same surface twice is a no-op.
    virtual void add_consumer(WlSurface& surface) = 0;
};

auto create_ext_foreign_buffer_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator)
    -> std::shared_ptr<wayland::ExtForeignBufferManagerV1::Global>;
}
}

#endif
