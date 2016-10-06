/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "render_surface.h"
#include "mir_wait_handle.h"
#include "rpc/mir_display_server.h"
#include "connection_surface_map.h"
#include "buffer_stream.h"
#include "error_stream.h"
#include "perf_report.h"
#include "logging/perf_report.h"
#include "lttng/perf_report.h"
#include "make_protobuf_object.h"
#include "mir_toolkit/mir_surface.h"
#include "mir/client_platform.h"
#include "mir_connection.h"

#include <algorithm>
#include <unistd.h>
#include <boost/exception/diagnostic_information.hpp>

namespace mcl = mir::client;
namespace mclr = mcl::rpc;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;
namespace mf = mir::frontend;
namespace ml = mir::logging;

mcl::RenderSurface::RenderSurface(
    MirConnection* const connection,
    mclr::DisplayServer& display_server,
    std::shared_ptr<ConnectionSurfaceMap> connection_surface_map,
    std::shared_ptr<void> native_window,
    std::shared_ptr<ClientPlatform> client_platform) :
        connection_(connection),
        server(display_server),
        surface_map(connection_surface_map),
        wrapped_native_window(native_window),
        platform(client_platform),
        stream_(nullptr),
        stream_creation_request(nullptr),
        stream_release_request(nullptr)
{
}

MirConnection* mcl::RenderSurface::connection() const
{
    return connection_;
}

namespace mir
{
namespace client
{
void render_surface_buffer_stream_create_callback(BufferStream* stream, void* context)
{
    mcl::RenderSurface::StreamCreationRequest* request{reinterpret_cast<mcl::RenderSurface::StreamCreationRequest*>(context)};
    mcl::RenderSurface* rs = request->rs;

    //TODO: check if there is no outstanding request already?

    rs->stream_ = dynamic_cast<ClientBufferStream*>(stream);
    if (request->usage == mir_buffer_usage_hardware)
    {
        rs->platform->use_egl_native_window(
            rs->wrapped_native_window, dynamic_cast<EGLNativeSurface*>(stream));
    }

    if (request->callback)
        request->callback(reinterpret_cast<MirBufferStream*>(rs->stream_), request->context);
}

void render_surface_buffer_stream_release_callback(MirBufferStream* stream, void* context)
{
    mcl::RenderSurface::StreamReleaseRequest* request{reinterpret_cast<mcl::RenderSurface::StreamReleaseRequest*>(context)};
    mcl::RenderSurface* rs = request->rs;

    //TODO: check if there is no outstanding request already?

    if (request->callback)
        request->callback(stream, request->context);

    rs->stream_ = nullptr;
}
}
}

MirWaitHandle* mcl::RenderSurface::create_client_buffer_stream(
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage usage,
    mir_buffer_stream_callback callback,
    void* context)
{
    // TODO: check if there is an outstanding stream request
    stream_creation_request = std::make_shared<StreamCreationRequest>(this, usage, callback, context);

    return connection_->create_client_buffer_stream(
        width, height, format, usage, this, nullptr,
        render_surface_buffer_stream_create_callback,
        stream_creation_request.get());
}

int mcl::RenderSurface::stream_id()
{
    return (stream_ ? stream_->rpc_id().as_value() : -1);
}

MirWaitHandle* mcl::RenderSurface::release_buffer_stream(
    mir_buffer_stream_callback callback,
    void* context)
{
    // TODO: check if there is an outstanding stream request
    stream_release_request = std::make_shared<StreamReleaseRequest>(callback, context, this);

    return connection_->release_buffer_stream(
        stream_, render_surface_buffer_stream_release_callback,
        stream_release_request.get());
}
