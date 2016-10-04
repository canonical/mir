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
#include "mir_protobuf.pb.h"
#include "mir/frontend/surface_id.h"
#include <mutex>
#include <memory>

namespace mir
{
namespace logging
{
class Logger;
}
namespace client
{
class ConnectionSurfaceMap;
class ClientPlatform;
class AsyncBufferFactory;
namespace rpc
{
class DisplayServer;
}
class RenderSurface : public MirRenderSurface
{
public:
    RenderSurface(int const width, int const height,
                  MirPixelFormat const format,
                  MirBufferUsage usage,
                  MirConnection* const connection,
                  rpc::DisplayServer& display_server,
                  std::shared_ptr<ConnectionSurfaceMap> connection_surface_map,
                  std::shared_ptr<void> native_window,
                  std::shared_ptr<ClientPlatform> client_platform);
    MirConnection* connection() const override;
    MirWaitHandle* create_client_buffer_stream(
        mir_buffer_stream_callback callback,
        void *context) override;
    int stream_id() override;

    MirWaitHandle* release_buffer_stream(
        void* native_surface,
        mir_render_surface_callback callback,
        void* context) override;

    friend void render_surface_buffer_stream_create_callback(BufferStream* stream, void* context);
    friend void render_surface_buffer_stream_release_callback(MirBufferStream* stream, void* context);

    struct StreamCreationRequest
    {
        StreamCreationRequest(
                mir_buffer_stream_callback cb, void* context, RenderSurface* rs) :
                    callback(cb),
                    context(context),
                    rs(rs)
        {
        }
        mir_buffer_stream_callback callback;
        void* context;
        RenderSurface* rs;
    };

    struct StreamReleaseRequest
    {
        StreamReleaseRequest(
                mir_render_surface_callback cb, void* context, RenderSurface* rs, void* native_surface) :
                    callback(cb),
                    context(context),
                    rs(rs),
                    native_surface(native_surface)

        {
        }
        mir_render_surface_callback callback;
        void* context;
        RenderSurface* rs;
        void* native_surface;
    };

private:
    int const width_, height_;
    MirPixelFormat const format_;
    MirBufferUsage buffer_usage;
    MirConnection* const connection_;
    rpc::DisplayServer& server;
    std::shared_ptr<ConnectionSurfaceMap> surface_map;
    std::shared_ptr<void> wrapped_native_window;
    std::shared_ptr<ClientPlatform> platform;
    ClientBufferStream* stream_;
    std::shared_ptr<StreamCreationRequest> stream_creation_request;
    std::shared_ptr<StreamReleaseRequest> stream_release_request;

    std::mutex mutex;
};
}
}
#endif /* MIR_CLIENT_RENDER_SURFACE_H */
