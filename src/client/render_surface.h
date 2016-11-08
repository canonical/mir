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
namespace client
{
class ClientPlatform;
namespace rpc
{
class DisplayServer;
}
class RenderSurface : public MirRenderSurface
{
public:
    RenderSurface(MirConnection* const connection,
                  std::shared_ptr<void> native_window,
                  std::shared_ptr<ClientPlatform> client_platform,
                  geometry::Size size);
    MirConnection* connection() const override;
    mir::geometry::Size size() const override;
    void set_size(mir::geometry::Size) override;
    MirWaitHandle* create_buffer_stream(
        int width, int height,
        MirPixelFormat format,
        MirBufferUsage usage,
        mir_buffer_stream_callback callback,
        void *context) override;
    mir::frontend::BufferStreamId stream_id() const override;

    MirWaitHandle* release_buffer_stream(
        mir_buffer_stream_callback callback,
        void* context) override;

    friend void render_surface_buffer_stream_create_callback(BufferStream* stream, void* context);
    friend void render_surface_buffer_stream_release_callback(MirBufferStream* stream, void* context);

    struct StreamCreationRequest
    {
        StreamCreationRequest(
                RenderSurface* rs,
                MirBufferUsage usage,
                mir_buffer_stream_callback cb,
                void* context) :
                    rs(rs),
                    usage(usage),
                    callback(cb),
                    context(context)
        {
        }
        RenderSurface* rs;
        MirBufferUsage usage;
        mir_buffer_stream_callback callback;
        void* context;
    };

    struct StreamReleaseRequest
    {
        StreamReleaseRequest(
                RenderSurface* rs,
                mir_buffer_stream_callback cb,
                void* context) :
                    rs(rs),
                    callback(cb),
                    context(context)
        {
        }
        RenderSurface* rs;
        mir_buffer_stream_callback callback;
        void* context;
    };

private:
    MirConnection* const connection_;
    std::shared_ptr<void> wrapped_native_window;
    std::shared_ptr<ClientPlatform> platform;
    ClientBufferStream* stream_;
    std::shared_ptr<StreamCreationRequest> stream_creation_request;
    std::shared_ptr<StreamReleaseRequest> stream_release_request;
    std::shared_timed_mutex guard;

    std::mutex mutable size_mutex;
    geometry::Size desired_size;
};
}
}
#endif /* MIR_CLIENT_RENDER_SURFACE_H */
