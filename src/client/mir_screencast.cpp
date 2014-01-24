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
#include "mir/frontend/client_constants.h"
#include "mir_toolkit/mir_native_buffer.h"

#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace geom = mir::geometry;

namespace
{

void null_callback(MirScreencast*, void*) {}

geom::Size mir_output_get_size(MirDisplayOutput const& output)
{
    if (output.connected && output.used &&
        output.current_mode < output.num_modes)
    {
        auto& current_mode = output.modes[output.current_mode];
        return geom::Size{current_mode.horizontal_resolution,
                          current_mode.vertical_resolution};
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Couldn't get size from invalid output"));
    }
}


void populate_buffer_package(
    MirBufferPackage& buffer_package,
    mir::protobuf::Buffer const& protobuf_buffer)
{
    if (!protobuf_buffer.has_error())
    {
        buffer_package.data_items = protobuf_buffer.data_size();
        for (int i = 0; i != protobuf_buffer.data_size(); ++i)
        {
            buffer_package.data[i] = protobuf_buffer.data(i);
        }

        buffer_package.fd_items = protobuf_buffer.fd_size();

        for (int i = 0; i != protobuf_buffer.fd_size(); ++i)
        {
            buffer_package.fd[i] = protobuf_buffer.fd(i);
        }

        buffer_package.stride = protobuf_buffer.stride();
        buffer_package.flags = protobuf_buffer.flags();
        buffer_package.width = protobuf_buffer.width();
        buffer_package.height = protobuf_buffer.height();
    }
    else
    {
        buffer_package.data_items = 0;
        buffer_package.fd_items = 0;
        buffer_package.stride = 0;
        buffer_package.flags = 0;
        buffer_package.width = 0;
        buffer_package.height = 0;
    }
}

}

MirScreencast::MirScreencast(
    MirDisplayOutput const& output,
    mir::protobuf::DisplayServer& server,
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    mir_screencast_callback callback, void* context)
    : server(server),
      output_id{output.output_id},
      output_size{mir_output_get_size(output)},
      output_format{output.current_format},
      buffer_depository{factory, mir::frontend::client_buffer_cache_size}
{
    next_buffer(callback, context);
}

MirWaitHandle* MirScreencast::creation_wait_handle()
{
    return &next_buffer_wait_handle;
}

MirSurfaceParameters MirScreencast::get_parameters() const
{
    return MirSurfaceParameters{
        "",
        output_size.width.as_int(),
        output_size.height.as_int(),
        output_format,
        mir_buffer_usage_hardware,
        output_id};
}

std::shared_ptr<mcl::ClientBuffer> MirScreencast::get_current_buffer()
{
    return buffer_depository.current_buffer();
}

MirWaitHandle* MirScreencast::next_buffer(
    mir_screencast_callback callback, void* context)
{
    mir::protobuf::ScreencastRequest request;
    request.set_output_id(output_id);

    server.screencast_buffer(
        nullptr,
        &request,
        &protobuf_buffer,
        google::protobuf::NewCallback(
            this, &MirScreencast::next_buffer_received,
            callback, context));

    return &next_buffer_wait_handle;
}

void MirScreencast::request_and_wait_for_next_buffer()
{
    next_buffer(null_callback, nullptr)->wait_for_all();
}

void MirScreencast::request_and_wait_for_configure(MirSurfaceAttrib, int)
{
}

void MirScreencast::next_buffer_received(
    mir_screencast_callback callback, void* context)
{
    auto buffer_package = std::make_shared<MirBufferPackage>();
    populate_buffer_package(*buffer_package, protobuf_buffer);

    try
    {
        buffer_depository.deposit_package(buffer_package,
                                          protobuf_buffer.buffer_id(),
                                          output_size, output_format);
    }
    catch (const std::runtime_error& err)
    {
        // TODO: Report the error
    }

    callback(this, context);
    next_buffer_wait_handle.result_received();
}
