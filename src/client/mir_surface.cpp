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
#include "client_buffer_factory.h"
#include "mir_surface.h"

#include <cassert>

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

MirSurface::MirSurface(
    mp::DisplayServer::Stub & server,
    const std::shared_ptr<mcl::ClientBufferFactory>& factory, 
    MirSurfaceParameters const & params,
    mir_surface_lifecycle_callback callback, void * context)
    : server(server),
      last_buffer_id(-1),
      surface_width(geom::Width{0}),
      surface_height(geom::Height{0}),
      surface_pf(geom::PixelFormat::rgba_8888), 
      buffer_factory(factory)
{
    mir::protobuf::SurfaceParameters message;
    message.set_surface_name(params.name ? params.name : std::string());
    message.set_width(params.width);
    message.set_height(params.height);
    message.set_pixel_format(params.pixel_format);

    server.create_surface(0, &message, &surface, gp::NewCallback(this, &MirSurface::created, callback, context));
}

MirWaitHandle* MirSurface::release(mir_surface_lifecycle_callback callback, void * context)
{
    release_cpu_region();

    mir::protobuf::SurfaceId message;
    message.set_value(surface.id().value());
    server.release_surface(0, &message, &void_response,
                           gp::NewCallback(this, &MirSurface::released, callback, context));
    return &release_wait_handle;
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

void MirSurface::get_cpu_region(MirGraphicsRegion& region_out)
{
    auto buffer = buffer_cache[last_buffer_id];
    secured_region = buffer->secure_for_cpu_write();
    region_out.width = secured_region->width.as_uint32_t();
    region_out.height = secured_region->height.as_uint32_t();
//    region_out.pixel_format = secured_region->pixel_format.as_uint32_t();
    //todo: fix
    region_out.pixel_format = mir_pixel_format_rgba_8888;
    region_out.vaddr = secured_region->vaddr.get();

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

void MirSurface::released(mir_surface_lifecycle_callback callback, void * context)
{
    callback(this, context);
    release_wait_handle.result_received();

    // we send release_wait_handle out to the client, so at this point, the client could still use it

    delete this;
}

/* todo: does this break single point of reference for width? */
void MirSurface::save_buffer_dimensions()
{
    surface_width = geom::Width(surface.width());
    surface_height = geom::Height(surface.height());
    //surface_pf = 
}

void MirSurface::created(mir_surface_lifecycle_callback callback, void * context)
{
    auto const& buffer = surface.buffer();
    last_buffer_id = buffer.buffer_id();

    save_buffer_dimensions();

    auto ipc_package = std::make_shared<MirBufferPackage>();
    populate(*ipc_package);

    auto new_buffer = buffer_factory->create_buffer_from_ipc_message(ipc_package, 
                                                                     surface_width, surface_height, surface_pf);

    /* this is only called when surface is first created. if anything has been putting things 
       in cache before this callback, its wrong */
    assert(buffer_cache.empty());
    buffer_cache[last_buffer_id] = new_buffer;

    callback(this, context);
    create_wait_handle.result_received();
}

void MirSurface::new_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    auto const& buffer = surface.buffer();
    last_buffer_id = buffer.buffer_id();

    auto it = buffer_cache.find(last_buffer_id);
    if (it == buffer_cache.end())
    {
        auto ipc_package = std::make_shared<MirBufferPackage>();
        populate(*ipc_package);
        auto new_buffer = buffer_factory->create_buffer_from_ipc_message(ipc_package, surface_width, surface_height, surface_pf);
        buffer_cache[last_buffer_id] = new_buffer;
    }
    
    callback(this, context);
    next_buffer_wait_handle.result_received();
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
