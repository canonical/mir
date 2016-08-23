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

#include <algorithm>
#include <unistd.h>
#include <boost/exception/diagnostic_information.hpp>

namespace mcl = mir::client;
namespace mclr = mcl::rpc;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;
namespace mf = mir::frontend;
namespace ml = mir::logging;

namespace
{
#if 0
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}
#endif

std::shared_ptr<mcl::PerfReport>
make_perf_report(std::shared_ptr<ml::Logger> const& logger)
{
    // TODO: It seems strange that this directly uses getenv
    const char* report_target = getenv("MIR_CLIENT_PERF_REPORT");
    if (report_target && !strcmp(report_target, "log"))
    {
        return std::make_shared<mcl::logging::PerfReport>(logger);
    }
    else if (report_target && !strcmp(report_target, "lttng"))
    {
        return std::make_shared<mcl::lttng::PerfReport>();
    }
    else
    {
        return std::make_shared<mcl::NullPerfReport>();
    }
}
}

mcl::RenderSurface::RenderSurface(
    int const width, int const height,
    MirPixelFormat const format,
    MirConnection* const connection,
    mclr::DisplayServer& display_server,
    std::shared_ptr<ConnectionSurfaceMap> connection_surface_map,
    std::shared_ptr<void> native_window,
    int num_buffers,
    std::shared_ptr<ClientPlatform> client_platform,
    std::shared_ptr<AsyncBufferFactory> async_buffer_factory,
    std::shared_ptr<ml::Logger> mir_logger) :
        width_(width),
        height_(height),
        format_(format),
        connection_(connection),
        server(display_server),
        surface_map(connection_surface_map),
        wrapped_native_window(native_window),
        nbuffers(num_buffers),
        platform(client_platform),
        buffer_factory(async_buffer_factory),
        logger(mir_logger),
        void_response(mcl::make_protobuf_object<mp::Void>()),
        container_(nullptr),
        autorelease_(false),
        stream_(nullptr)
{
}

MirConnection* mcl::RenderSurface::connection() const
{
    return connection_;
}

MirWaitHandle* mcl::RenderSurface::create_client_buffer_stream(
    MirBufferUsage buffer_usage,
    mir_buffer_stream_callback callback,
    bool autorelease,
    void* context)
{
    mp::BufferStreamParameters params;
    params.set_width(width_);
    params.set_height(height_);
    params.set_pixel_format(format_);
    params.set_buffer_usage(buffer_usage);

    autorelease_ = autorelease;

    auto request = std::make_shared<StreamCreationRequest>(callback, context, params);
    request->wh->expect_result();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        stream_requests.push_back(request);
    }

    try
    {
        server.create_buffer_stream(&params, request->response.get(),
            gp::NewCallback(this, &mcl::RenderSurface::stream_created, request.get()));
    } catch (std::exception& e)
    {
        //if this throws, our socket code will run the closure, which will make an error object.
        //its nicer to return a stream with an error message, so just ignore the exception.
    }

    return request->wh.get();
}

mf::SurfaceId mcl::RenderSurface::next_error_id(std::unique_lock<std::mutex> const&)
{
    return mf::SurfaceId{stream_error_id--};
}

void mcl::RenderSurface::stream_error(std::string const& error_msg, std::shared_ptr<StreamCreationRequest> const& request)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    mf::BufferStreamId id(next_error_id(lock).as_value());
    auto stream = std::make_shared<mcl::ErrorStream>(error_msg, this, id, request->wh);
    surface_map->insert(id, stream);

    if (request->callback)
    {
        request->callback(reinterpret_cast<MirBufferStream*>(dynamic_cast<mcl::ClientBufferStream*>(stream.get())), request->context);
    }
    request->wh->result_received();
}

void mcl::RenderSurface::stream_created(StreamCreationRequest* request_raw)
{
    std::shared_ptr<StreamCreationRequest> request {nullptr};
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto stream_it = std::find_if(stream_requests.begin(), stream_requests.end(),
            [&request_raw] (std::shared_ptr<StreamCreationRequest> const& req)
            { return req.get() == request_raw; });
        if (stream_it == stream_requests.end())
            return;
        request = *stream_it;
        stream_requests.erase(stream_it);
    }

    auto& protobuf_bs = request->response;
    if (!protobuf_bs->has_id())
    {
        if (!protobuf_bs->has_error())
            protobuf_bs->set_error("no ID in response (disconnected?)");
    }

    if (protobuf_bs->has_error())
    {
        for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
            ::close(protobuf_bs->buffer().fd(i));
        stream_error(
            std::string{"Error processing buffer stream response: "} + protobuf_bs->error(),
            request);
        return;
    }

    try
    {
        auto stream = std::make_shared<mcl::BufferStream>(
            connection(), request->wh, server, platform, surface_map, buffer_factory,
            *protobuf_bs, make_perf_report(logger), std::string{},
            mir::geometry::Size{request->parameters.width(), request->parameters.height()}, nbuffers, nullptr);
        id = protobuf_bs->id().value();
        surface_map->insert(mf::BufferStreamId(id), stream);

        stream_ = stream.get();
        platform->use_egl_native_window(wrapped_native_window, stream.get());
        if (request->callback)
            request->callback(reinterpret_cast<MirBufferStream*>(dynamic_cast<mcl::ClientBufferStream*>(stream.get())), request->context);
        request->wh->result_received();
    }
    catch (std::exception const& error)
    {
        for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
            ::close(protobuf_bs->buffer().fd(i));

        stream_error(
            std::string{"Error processing buffer stream creating response:"} + boost::diagnostic_information(error),
            request);
    }
}

int mcl::RenderSurface::stream_id()
{
    return id;
}

bool mcl::RenderSurface::autorelease_content() const
{
    return autorelease_;
}

struct mcl::RenderSurface::StreamRelease
{
    ClientBufferStream* stream;
    MirWaitHandle* handle;
    mir_buffer_stream_callback callback;
    void* context;
    int rpc_id;
};

MirWaitHandle* mcl::RenderSurface::release_buffer_stream(
    mir_buffer_stream_callback callback,
    void* context)
{
    auto new_wait_handle = new MirWaitHandle;

    StreamRelease stream_release{stream_, new_wait_handle, callback, context, stream_->rpc_id().as_value() };

    mp::BufferStreamId buffer_stream_id;
    buffer_stream_id.set_value(stream_->rpc_id().as_value());

    {
        std::lock_guard<decltype(release_wait_handle_guard)> rel_lock(release_wait_handle_guard);
        release_wait_handles.push_back(new_wait_handle);
    }

    new_wait_handle->expect_result();
    server.release_buffer_stream(
        &buffer_stream_id, void_response.get(),
        google::protobuf::NewCallback(this, &RenderSurface::released, stream_release));
    return new_wait_handle;
}

void mcl::RenderSurface::released(StreamRelease data)
{
    if (data.callback)
        data.callback(reinterpret_cast<MirBufferStream*>(data.stream), data.context);
    if (data.handle)
        data.handle->result_received();
    stream_ = nullptr;
    surface_map->erase(mf::BufferStreamId(data.rpc_id));
}

#if 0
void mcl::RenderSurface::set_container(MirSurface* const surface)
{
    container_ = surface;
}

MirSurface* mcl::RenderSurface::container()
{
    return container_;
}

MirEGLNativeWindowType mcl::RenderSurface::egl_native_window()
{
    MirBufferStream* stream = nullptr;
    auto wh = create_client_buffer_stream(
        100,
        100,
        mir_pixel_format_argb_8888,
        mir_buffer_usage_hardware,
        reinterpret_cast<mir_buffer_stream_callback>(assign_result),
        &stream);
    wh->wait_for_all();
    auto spec = mir_connection_create_spec_for_changes(connection_);
    mir_surface_spec_add_render_surface(spec, 100, 100, 0, 0, this);
    mir_surface_apply_spec(container_, spec);
    mir_surface_spec_release(spec);
    mcl::ClientBufferStream *bs = reinterpret_cast<mcl::ClientBufferStream*>(stream);
    return reinterpret_cast<MirEGLNativeWindowType>(bs->egl_native_window());
}
#endif
