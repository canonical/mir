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

#include "mir_client/private/mir_surface.h"
#include "mir_client/private/client_buffer_factory.h"

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

mcl::MirSurface::MirSurface(
    mp::DisplayServer::Stub & server,
    const std::shared_ptr<ClientBufferFactory>& /* factory */, 
    MirSurfaceParameters const & params,
    mir_surface_lifecycle_callback callback, void * context)
    : server(server)
{
    mir::protobuf::SurfaceParameters message;
    message.set_surface_name(params.name ? params.name : std::string());
    message.set_width(params.width);
    message.set_height(params.height);
    message.set_pixel_format(params.pixel_format);

    create_wait_handle.result_requested();
    server.create_surface(0, &message, &surface, gp::NewCallback(this, &MirSurface::created, callback, context));
}

mcl::MirWaitHandle* mcl::MirSurface::release(mir_surface_lifecycle_callback callback, void * context)
{
    mir::protobuf::SurfaceId message;
    message.set_value(surface.id().value());
    release_wait_handle.result_requested();
    server.release_surface(0, &message, &void_response,
                           gp::NewCallback(this, &MirSurface::released, callback, context));
    return &release_wait_handle;
}

MirSurfaceParameters mcl::MirSurface::get_parameters() const
{
    return MirSurfaceParameters {
        0,
        surface.width(),
        surface.height(),
        static_cast<MirPixelFormat>(surface.pixel_format())};
}

char const * mcl::MirSurface::get_error_message()
{
    if (surface.has_error())
    {
        return surface.error().c_str();
    }
    return error_message.c_str();
}

int mcl::MirSurface::id() const
{
    return surface.id().value();
}

bool mcl::MirSurface::is_valid() const
{
    return !surface.has_error();
}

void mcl::MirSurface::populate(MirGraphicsRegion& )
{
    // TODO
}

mcl::MirWaitHandle* mcl::MirSurface::next_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    next_buffer_wait_handle.result_requested();
    server.next_buffer(
        0,
        &surface.id(),
        surface.mutable_buffer(),
        google::protobuf::NewCallback(this, &MirSurface::new_buffer, callback, context));

    return &next_buffer_wait_handle;
}

mcl::MirWaitHandle* mcl::MirSurface::get_create_wait_handle()
{
    return &create_wait_handle;
}

void mcl::MirSurface::released(mir_surface_lifecycle_callback callback, void * context)
{
    auto cast = ( ::MirSurface* ) this;
    callback(cast, context);
    release_wait_handle.result_received();
    delete this;
}

void mcl::MirSurface::created(mir_surface_lifecycle_callback callback, void * context)
{
    auto cast = ( ::MirSurface* ) this;
    callback(cast , context);
    create_wait_handle.result_received();
}

void mcl::MirSurface::new_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    auto cast = ( ::MirSurface* ) this;
    callback(cast, context);
    next_buffer_wait_handle.result_received();
}

