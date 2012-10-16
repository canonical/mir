/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_client/mir_client_library.h"
#include "client_buffer.h"
#include "mir_surface.h"
#include "mir_connection.h"

#include <cassert>

namespace mp = mir::protobuf;
namespace gp = google::protobuf;

MirSurface::MirSurface(
    MirConnection * allocating_connection,
    mp::DisplayServer::Stub & server,
    MirSurfaceParameters const & params,
    mir_surface_lifecycle_callback callback, void * context)
    : server(server),
      connection(allocating_connection)
{
    mir::protobuf::SurfaceParameters message;
    message.set_surface_name(params.name ? params.name : std::string());
    message.set_width(params.width);
    message.set_height(params.height);
    message.set_pixel_format(params.pixel_format);

    server.create_surface(0, &message, &surface, gp::NewCallback(this, &MirSurface::created, callback, context));
}

MirSurface::~MirSurface()
{
    release_cpu_region();
}

MirSurfaceParameters MirSurface::get_parameters() const
{
    return MirSurfaceParameters {
        0,
        surface.width(),
        surface.height(),
        static_cast<MirPixelFormat>(surface.pixel_format())};
}

char const * MirSurface::get_error_message()
{
    if (surface.has_error())
    {
        return surface.error().c_str();
    }
    return error_message.c_str();
}

int MirSurface::id() const
{
    return surface.id().value();
}

bool MirSurface::is_valid() const
{
    return !surface.has_error();
}

void MirSurface::get_cpu_region(MirGraphicsRegion& /*region_out*/)
{
}

void MirSurface::release_cpu_region()
{
    secured_region.reset();
}

MirWaitHandle* MirSurface::next_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    release_cpu_region();

    server.next_buffer(
        0,
        &surface.id(),
        surface.mutable_buffer(),
        google::protobuf::NewCallback(this, &MirSurface::new_buffer, callback, context));
    
    return &next_buffer_wait_handle;
}

MirWaitHandle* MirSurface::get_create_wait_handle()
{
    return &create_wait_handle;
}

void MirSurface::created(mir_surface_lifecycle_callback callback, void * context)
{
    callback(this, context);
    create_wait_handle.result_received();
}

void MirSurface::new_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    callback(this, context);
    next_buffer_wait_handle.result_received();
}

MirWaitHandle* MirSurface::release_surface(
        MirSurface *surface,
        mir_surface_lifecycle_callback callback,
        void * context)
{
    return connection->release_surface(surface, callback, context); 
}

void MirSurface::populate(MirBufferPackage& buffer_package)
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
    }
    else
    {
        buffer_package.data_items = 0;
        buffer_package.fd_items = 0;
    }
}
