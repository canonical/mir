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
#include "mir/egl_native_window_factory.h"

#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace geom = mir::geometry;

namespace
{

void null_callback(MirScreencast*, void*) {}

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
    geom::Rectangle const& region,
    geom::Size const& size,
    MirPixelFormat pixel_format,
    mir::protobuf::DisplayServer& server,
    std::shared_ptr<mcl::EGLNativeWindowFactory> const& egl_native_window_factory,
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    mir_screencast_callback callback, void* context)
    : server(server),
      output_size{size},
      output_format{pixel_format},
      egl_native_window_factory{egl_native_window_factory},
      buffer_depository{factory, mir::frontend::client_buffer_cache_size}
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
    return MirSurfaceParameters{
        "",
        output_size.width.as_int(),
        output_size.height.as_int(),
        output_format,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid};
}

std::shared_ptr<mcl::ClientBuffer> MirScreencast::get_current_buffer()
{
    return buffer_depository.current_buffer();
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
    mir::protobuf::ScreencastId screencast_id;
    screencast_id.set_value(protobuf_screencast.screencast_id().value());

    next_buffer_wait_handle.expect_result();
    server.screencast_buffer(
        nullptr,
        &screencast_id,
        &protobuf_buffer,
        google::protobuf::NewCallback(
            this, &MirScreencast::next_buffer_received,
            callback, context));

    return &next_buffer_wait_handle;
}

EGLNativeWindowType MirScreencast::egl_native_window()
{
    return *egl_native_window_;
}

void MirScreencast::request_and_wait_for_next_buffer()
{
    next_buffer(null_callback, nullptr)->wait_for_all();
}

void MirScreencast::request_and_wait_for_configure(MirSurfaceAttrib, int)
{
}

void MirScreencast::process_buffer(mir::protobuf::Buffer const& buffer)
{
    auto buffer_package = std::make_shared<MirBufferPackage>();
    populate_buffer_package(*buffer_package, buffer);

    try
    {
        buffer_depository.deposit_package(buffer_package,
                                          buffer.buffer_id(),
                                          output_size, output_format);
    }
    catch (const std::runtime_error& err)
    {
        // TODO: Report the error
    }
}

void MirScreencast::screencast_created(
    mir_screencast_callback callback, void* context)
{
    if (!protobuf_screencast.has_error())
    {
        egl_native_window_ = egl_native_window_factory->create_egl_native_window(this);
        process_buffer(protobuf_screencast.buffer());
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

void MirScreencast::next_buffer_received(
    mir_screencast_callback callback, void* context)
{
    process_buffer(protobuf_buffer);

    callback(this, context);
    next_buffer_wait_handle.result_received();
}
