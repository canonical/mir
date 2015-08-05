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
#include "mir_connection.h"
#include "mir_protobuf.pb.h"
#include "make_protobuf_object.h"
#include "client_buffer_stream.h"
#include "mir/frontend/client_constants.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir/egl_native_window_factory.h"

#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;

MirScreencast::MirScreencast(
    geom::Rectangle const& region,
    geom::Size const& size,
    MirPixelFormat pixel_format,
    mir::client::rpc::DisplayServer& server,
    MirConnection* connection,
    mir_screencast_callback callback, void* context)
    : server(server),
      connection{connection},
      output_size{size},
      protobuf_screencast{mcl::make_protobuf_object<mir::protobuf::Screencast>()},
      protobuf_void{mcl::make_protobuf_object<mir::protobuf::Void>()}
{
    if (output_size.width.as_int()  == 0 ||
        output_size.height.as_int() == 0 ||
        region.size.width.as_int()  == 0 ||
        region.size.height.as_int() == 0 ||
        pixel_format == mir_pixel_format_invalid)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid parameters"));
    }
    protobuf_screencast->set_error("Not initialized");

    mp::ScreencastParameters parameters;

    parameters.mutable_region()->set_left(region.top_left.x.as_int());
    parameters.mutable_region()->set_top(region.top_left.y.as_int());
    parameters.mutable_region()->set_width(region.size.width.as_uint32_t());
    parameters.mutable_region()->set_height(region.size.height.as_uint32_t());
    parameters.set_width(output_size.width.as_uint32_t());
    parameters.set_height(output_size.height.as_uint32_t());
    parameters.set_pixel_format(pixel_format);

    create_screencast_wait_handle.expect_result();
    server.create_screencast(
        &parameters,
        protobuf_screencast.get(),
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
    return !protobuf_screencast->has_error();
}

MirWaitHandle* MirScreencast::release(
        mir_screencast_callback callback, void* context)
{
    mp::ScreencastId screencast_id;
    screencast_id.set_value(protobuf_screencast->screencast_id().value());
    
    release_wait_handle.expect_result();
    server.release_screencast(
        &screencast_id,
        protobuf_void.get(),
        google::protobuf::NewCallback(
            this, &MirScreencast::released, callback, context));

    return &release_wait_handle;
}

void MirScreencast::request_and_wait_for_configure(MirSurfaceAttrib, int)
{
}

void MirScreencast::screencast_created(
    mir_screencast_callback callback, void* context)
{
    if (!protobuf_screencast->has_error() && connection)
    {
        buffer_stream = connection->make_consumer_stream(
            protobuf_screencast->buffer_stream(), "MirScreencast");
    }

    callback(this, context);
    create_screencast_wait_handle.result_received();
}

void MirScreencast::released(
    mir_screencast_callback callback, void* context)
{
    callback(this, context);
    if (connection)
        connection->release_consumer_stream(buffer_stream.get());
    buffer_stream.reset();

    release_wait_handle.result_received();
}

mir::client::ClientBufferStream* MirScreencast::get_buffer_stream()
{
    return buffer_stream.get();
}
