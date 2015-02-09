/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_screencast.h"
#include "client_buffer_stream_factory.h"
#include "client_buffer_stream.h"
#include "mir/frontend/client_constants.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir/egl_native_window_factory.h"

#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;

namespace
{

void null_callback(MirScreencast*, void*) {}

}

MirScreencast::MirScreencast(
    geom::Rectangle const& region,
    geom::Size const& size,
    MirPixelFormat pixel_format,
    mir::protobuf::DisplayServer& server,
    std::shared_ptr<mcl::ClientBufferStreamFactory> const& buffer_stream_factory,
    mir_screencast_callback callback, void* context)
    : server(server),
      output_size{size},
      buffer_stream_factory{buffer_stream_factory}
{
    if (output_size.width.as_int()  == 0 ||
        output_size.height.as_int() == 0 ||
        region.size.width.as_int()  == 0 ||
        region.size.height.as_int() == 0 ||
        pixel_format == mir_pixel_format_invalid)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid parameters"));
    }
    protobuf_screencast.set_error("Not initialized");

    mir::protobuf::ScreencastParameters parameters;

    parameters.mutable_region()->set_left(region.top_left.x.as_int());
    parameters.mutable_region()->set_top(region.top_left.y.as_int());
    parameters.mutable_region()->set_width(region.size.width.as_uint32_t());
    parameters.mutable_region()->set_height(region.size.height.as_uint32_t());
    parameters.set_width(output_size.width.as_uint32_t());
    parameters.set_height(output_size.height.as_uint32_t());
    parameters.set_pixel_format(pixel_format);

    create_screencast_wait_handle.expect_result();
    server.create_screencast(
        nullptr,
        &parameters,
        &protobuf_screencast,
        google::protobuf::NewCallback(
            this, &MirScreencast::screencast_created,
            callback, context));
}

MirWaitHandle* MirScreencast::creation_wait_handle()
{
    return &create_screencast_wait_handle;
}

bool MirScreencast::valid()
{
    return !protobuf_screencast.has_error();
}

MirSurfaceParameters MirScreencast::get_parameters() const
{
    return buffer_stream->get_parameters();
}

std::shared_ptr<mcl::ClientBuffer> MirScreencast::get_current_buffer()
{
    return buffer_stream->get_current_buffer();
}

MirWaitHandle* MirScreencast::release(
        mir_screencast_callback callback, void* context)
{
    mir::protobuf::ScreencastId screencast_id;
    screencast_id.set_value(protobuf_screencast.screencast_id().value());
    
    release_wait_handle.expect_result();
    server.release_screencast(
        nullptr,
        &screencast_id,
        &protobuf_void,
        google::protobuf::NewCallback(
            this, &MirScreencast::released, callback, context));

    return &release_wait_handle;
}

MirWaitHandle* MirScreencast::next_buffer(
    mir_screencast_callback callback, void* context)
{
    return buffer_stream->next_buffer([&, callback, context]() {
        if (callback)
            callback(this, context);
    });
}

EGLNativeWindowType MirScreencast::egl_native_window()
{
    return buffer_stream->egl_native_window();
}

void MirScreencast::request_and_wait_for_next_buffer()
{
    next_buffer(null_callback, nullptr)->wait_for_all();
}

void MirScreencast::request_and_wait_for_configure(MirSurfaceAttrib, int)
{
}

void MirScreencast::screencast_created(
    mir_screencast_callback callback, void* context)
{
    if (!protobuf_screencast.has_error())
    {
        buffer_stream = buffer_stream_factory->make_consumer_stream(server,
            protobuf_screencast.buffer_stream());
    }

    callback(this, context);
    create_screencast_wait_handle.result_received();
}

void MirScreencast::released(
    mir_screencast_callback callback, void* context)
{
    callback(this, context);
    release_wait_handle.result_received();
}
