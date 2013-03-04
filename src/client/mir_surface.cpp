/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_logger.h"
#include "client_buffer.h"
#include "mir_surface.h"
#include "mir_connection.h"

#include <cassert>

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

mir_toolkit::MirSurface::MirSurface(
    MirConnection *allocating_connection,
    mp::DisplayServer::Stub & server,
    const std::shared_ptr<mir::client::Logger>& logger,
    const std::shared_ptr<mcl::ClientBufferDepository>& depository,
    MirSurfaceParameters const & params,
    mir_surface_lifecycle_callback callback, void * context)
    : server(server),
      connection(allocating_connection),
      last_buffer_id(-1),
      buffer_depository(depository),
      logger(logger)
{
    mir::protobuf::SurfaceParameters message;
    message.set_surface_name(params.name ? params.name : std::string());
    message.set_width(params.width);
    message.set_height(params.height);
    message.set_pixel_format(params.pixel_format);
    message.set_buffer_usage(params.buffer_usage);

    server.create_surface(0, &message, &surface, gp::NewCallback(this, &MirSurface::created, callback, context));
}

mir_toolkit::MirSurface::~MirSurface()
{
    release_cpu_region();
}

mir_toolkit::MirSurfaceParameters mir_toolkit::MirSurface::get_parameters() const
{
    return MirSurfaceParameters {
        0,
        surface.width(),
        surface.height(),
        static_cast<MirPixelFormat>(surface.pixel_format()),
        static_cast<MirBufferUsage>(surface.buffer_usage())};
}

char const * mir_toolkit::MirSurface::get_error_message()
{
    if (surface.has_error())
    {
        return surface.error().c_str();
    }
    return error_message.c_str();
}

int mir_toolkit::MirSurface::id() const
{
    return surface.id().value();
}

bool mir_toolkit::MirSurface::is_valid() const
{
    return !surface.has_error();
}

void mir_toolkit::MirSurface::get_cpu_region(MirGraphicsRegion& region_out)
{
    auto buffer = buffer_depository->access_buffer(last_buffer_id);

    secured_region = buffer->secure_for_cpu_write();
    region_out.width = secured_region->width.as_uint32_t();
    region_out.height = secured_region->height.as_uint32_t();
    region_out.stride = secured_region->stride.as_uint32_t();
    //todo: fix
    region_out.pixel_format = mir_pixel_format_abgr_8888;

    region_out.vaddr = secured_region->vaddr.get();
}

void mir_toolkit::MirSurface::release_cpu_region()
{
    secured_region.reset();
}

mir_toolkit::MirWaitHandle* mir_toolkit::MirSurface::next_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    release_cpu_region();

    server.next_buffer(
        0,
        &surface.id(),
        surface.mutable_buffer(),
        google::protobuf::NewCallback(this, &MirSurface::new_buffer, callback, context));

    return &next_buffer_wait_handle;
}

mir_toolkit::MirWaitHandle* mir_toolkit::MirSurface::get_create_wait_handle()
{
    return &create_wait_handle;
}

/* todo: all these conversion functions are a bit of a kludge, probably
         better to have a more developed geometry::PixelFormat that can handle this */
geom::PixelFormat mir_toolkit::MirSurface::convert_ipc_pf_to_geometry(gp::int32 pf )
{
    if ( pf == mir_pixel_format_abgr_8888 )
        return geom::PixelFormat::abgr_8888;
    return geom::PixelFormat::invalid;
}

void mir_toolkit::MirSurface::process_incoming_buffer()
{
    auto const& buffer = surface.buffer();
    last_buffer_id = buffer.buffer_id();

    auto surface_width = geom::Width(surface.width());
    auto surface_height = geom::Height(surface.height());
    auto surface_size = geom::Size{surface_width, surface_height};
    auto surface_pf = convert_ipc_pf_to_geometry(surface.pixel_format());

    auto ipc_package = std::make_shared<MirBufferPackage>();
    populate(*ipc_package);

    try
    {
        buffer_depository->deposit_package(std::move(ipc_package),
                                last_buffer_id,
                                surface_size, surface_pf);
    } catch (const std::runtime_error& err)
    {
        logger->error() << err.what();
    }
}

void mir_toolkit::MirSurface::created(mir_surface_lifecycle_callback callback, void * context)
{
    process_incoming_buffer();

    auto platform = connection->get_client_platform();
    accelerated_window = platform->create_egl_native_window(this);

    callback(this, context);
    create_wait_handle.result_received();
}

void mir_toolkit::MirSurface::new_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    process_incoming_buffer();

    callback(this, context);
    next_buffer_wait_handle.result_received();
}

mir_toolkit::MirWaitHandle* mir_toolkit::MirSurface::release_surface(
        mir_surface_lifecycle_callback callback,
        void * context)
{
    return connection->release_surface(this, callback, context);
}

std::shared_ptr<mir_toolkit::MirBufferPackage> mir_toolkit::MirSurface::get_current_buffer_package()
{
    auto buffer = buffer_depository->access_buffer(last_buffer_id);
    return buffer->get_buffer_package();
}

std::shared_ptr<mcl::ClientBuffer> mir_toolkit::MirSurface::get_current_buffer()
{
    return buffer_depository->access_buffer(last_buffer_id);
}

void mir_toolkit::MirSurface::populate(MirBufferPackage& buffer_package)
{
    if (is_valid() && surface.has_buffer())
    {
        auto const& buffer = surface.buffer();

        buffer_package.data_items = buffer.data_size();
        for (int i = 0; i != buffer.data_size(); ++i)
        {
            buffer_package.data[i] = buffer.data(i);
        }

        buffer_package.fd_items = buffer.fd_size();

        for (int i = 0; i != buffer.fd_size(); ++i)
        {
            buffer_package.fd[i] = buffer.fd(i);
        }

        buffer_package.stride = buffer.stride();
    }
    else
    {
        buffer_package.data_items = 0;
        buffer_package.fd_items = 0;
        buffer_package.stride = 0;
    }
}

EGLNativeWindowType mir_toolkit::MirSurface::generate_native_window()
{
    return *accelerated_window;
}
