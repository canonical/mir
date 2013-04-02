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
#include "input/input_receiver_thread.h"
#include "input/input_platform.h"

#include <cassert>

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mcli = mir::client::input;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

MirSurface::MirSurface(
    MirConnection *allocating_connection,
    mp::DisplayServer::Stub & server,
    std::shared_ptr<mir::client::Logger> const& logger,
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    std::shared_ptr<mcli::InputPlatform> const& input_platform,
    MirSurfaceParameters const & params,
    MirEventDelegate const* delegate,
    mir_surface_lifecycle_callback callback, void * context)
    : server(server),
      connection(allocating_connection),
      buffer_depository(std::make_shared<mcl::ClientBufferDepository>(factory, 3)),
      input_platform(input_platform),
      logger(logger)
{
    mir::protobuf::SurfaceParameters message;
    message.set_surface_name(params.name ? params.name : std::string());
    message.set_width(params.width);
    message.set_height(params.height);
    message.set_pixel_format(params.pixel_format);
    message.set_buffer_usage(params.buffer_usage);
    
    if (delegate)
        handle_event_callback = std::bind(delegate->handle_input, this, std::placeholders::_1, delegate->context);

    server.create_surface(0, &message, &surface, gp::NewCallback(this, &MirSurface::created, callback, context));

    for (int i = 0; i < mir_surface_attrib_arraysize_; i++)
        attrib_cache[i] = -1;
    attrib_cache[mir_surface_attrib_type] = mir_surface_type_normal;
}

MirSurface::~MirSurface()
{
    if (input_thread)
        input_thread->stop();
    release_cpu_region();
}

MirSurfaceParameters MirSurface::get_parameters() const
{
    return MirSurfaceParameters {
        0,
        surface.width(),
        surface.height(),
        static_cast<MirPixelFormat>(surface.pixel_format()),
        static_cast<MirBufferUsage>(surface.buffer_usage())};
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
    auto buffer = buffer_depository->current_buffer();

    secured_region = buffer->secure_for_cpu_write();
    region_out.width = secured_region->width.as_uint32_t();
    region_out.height = secured_region->height.as_uint32_t();
    region_out.stride = secured_region->stride.as_uint32_t();
    //todo: fix
    region_out.pixel_format = mir_pixel_format_abgr_8888;

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

/* todo: all these conversion functions are a bit of a kludge, probably
         better to have a more developed geometry::PixelFormat that can handle this */
geom::PixelFormat MirSurface::convert_ipc_pf_to_geometry(gp::int32 pf)
{
    if (pf == mir_pixel_format_abgr_8888)
        return geom::PixelFormat::abgr_8888;
    return geom::PixelFormat::invalid;
}

void MirSurface::process_incoming_buffer()
{
    auto const& buffer = surface.buffer();

    auto surface_width = geom::Width(surface.width());
    auto surface_height = geom::Height(surface.height());
    auto surface_size = geom::Size{surface_width, surface_height};
    auto surface_pf = convert_ipc_pf_to_geometry(surface.pixel_format());

    auto ipc_package = std::make_shared<MirBufferPackage>();
    populate(*ipc_package);

    try
    {
        buffer_depository->deposit_package(std::move(ipc_package),
                                buffer.buffer_id(),
                                surface_size, surface_pf);
    } catch (const std::runtime_error& err)
    {
        logger->error() << err.what();
    }
}

void MirSurface::created(mir_surface_lifecycle_callback callback, void * context)
{
    process_incoming_buffer();

    auto platform = connection->get_client_platform();
    accelerated_window = platform->create_egl_native_window(this);

    callback(this, context);
    
    if (surface.fd_size() > 0 && handle_event_callback)
    {
        input_thread = input_platform->create_input_thread(surface.fd(0), handle_event_callback);
        input_thread->start();
    }

    create_wait_handle.result_received();
}

void MirSurface::new_buffer(mir_surface_lifecycle_callback callback, void * context)
{
    process_incoming_buffer();

    callback(this, context);
    next_buffer_wait_handle.result_received();
}

MirWaitHandle* MirSurface::release_surface(
        mir_surface_lifecycle_callback callback,
        void * context)
{
    return connection->release_surface(this, callback, context);
}

std::shared_ptr<MirBufferPackage> MirSurface::get_current_buffer_package()
{
    auto buffer = buffer_depository->current_buffer();
    return buffer->get_buffer_package();
}

std::shared_ptr<mcl::ClientBuffer> MirSurface::get_current_buffer()
{
    return buffer_depository->current_buffer();
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

        buffer_package.stride = buffer.stride();
    }
    else
    {
        buffer_package.data_items = 0;
        buffer_package.fd_items = 0;
        buffer_package.stride = 0;
    }
}

EGLNativeWindowType MirSurface::generate_native_window()
{
    return *accelerated_window;
}

MirWaitHandle* MirSurface::configure(MirSurfaceAttrib at, int value)
{
    mp::SurfaceSetting setting;
    setting.mutable_surfaceid()->CopyFrom(surface.id());
    setting.set_attrib(at);
    setting.set_ivalue(value);

    configure_wait_handle.expect_result();
    server.configure_surface(0, &setting, &configure_result,
              google::protobuf::NewCallback(this, &MirSurface::on_configured));

    return &configure_wait_handle;
}

void MirSurface::on_configured()
{
    if (configure_result.has_surfaceid() &&
        configure_result.surfaceid().value() == surface.id().value() &&
        configure_result.has_attrib())
    {
        switch (configure_result.attrib())
        {
        case mir_surface_attrib_type:
            if (configure_result.has_ivalue())
            {
                int t = configure_result.ivalue();
                attrib_cache[mir_surface_attrib_type] = t;
            } // else error is probably set due to an unsupported attrib/value
            break;
        default:
            assert(false);
            break;
        }

        configure_wait_handle.result_received();
    }
}

int MirSurface::attrib(MirSurfaceAttrib at) const
{
    return attrib_cache[at];
}
