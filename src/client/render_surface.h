/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_CLIENT_RENDER_SURFACE_H
#define MIR_CLIENT_RENDER_SURFACE_H

#include "mir_connection.h"
#include "mir_render_surface.h"
#include "mir_toolkit/mir_render_surface.h"
#include "mir_toolkit/client_types.h"
#include "mir/frontend/surface_id.h"

#include <shared_mutex>
#include <memory>

namespace mir
{
namespace protobuf
{
class BufferStream;
}
namespace client
{
class ClientPlatform;
class BufferStream;
class RenderSurface : public MirRenderSurface
{
public:
    RenderSurface(MirConnection* const connection,
                  std::shared_ptr<void> native_window,
                  std::shared_ptr<ClientPlatform> client_platform,
                  std::shared_ptr<mir::protobuf::BufferStream> protobuf_bs,
                  geometry::Size size);
    MirConnection* connection() const override;
    mir::geometry::Size size() const override;
    void set_size(mir::geometry::Size) override;
    mir::frontend::BufferStreamId stream_id() const override;

    MirBufferStream* create_buffer_stream_from_id(
        int width, int height,
        MirPixelFormat format,
        MirBufferUsage buffer_usage) override;

private:
    MirConnection* const connection_;
    std::shared_ptr<void> wrapped_native_window;
    std::shared_ptr<ClientPlatform> platform;
    std::shared_ptr<mir::protobuf::BufferStream> protobuf_bs;
    std::shared_ptr<mir::client::BufferStream> stream_from_id;

    std::mutex mutable size_mutex;
    geometry::Size desired_size;
};
}
}
#endif /* MIR_CLIENT_RENDER_SURFACE_H */
