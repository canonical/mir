/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/session_mediator.h"
#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/shell.h"
#include "mir/frontend/session.h"
#include "mir/frontend/surface.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/frontend/resource_cache.h"
#include "mir_toolkit/common.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/buffer_id.h"
#include "mir/compositor/buffer.h"
#include "mir/surfaces/buffer_bundle.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/viewable_area.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/frontend/client_constants.h"
#include "client_buffer_tracker.h"

#include <boost/throw_exception.hpp>

mir::frontend::SessionMediator::SessionMediator(
    std::shared_ptr<frontend::Shell> const& shell,
    std::shared_ptr<graphics::Platform> const & graphics_platform,
    std::shared_ptr<graphics::ViewableArea> const& viewable_area,
    std::shared_ptr<compositor::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<SessionMediatorReport> const& report,
    std::shared_ptr<events::EventSink> const& event_sink,
    std::shared_ptr<ResourceCache> const& resource_cache) :
    shell(shell),
    graphics_platform(graphics_platform),
    viewable_area(viewable_area),
    buffer_allocator(buffer_allocator),
    report(report),
    event_sink(event_sink),
    resource_cache(resource_cache),
    client_tracker(std::make_shared<ClientBufferTracker>(frontend::client_buffer_cache_size))
{
}

void mir::frontend::SessionMediator::connect(
    ::google::protobuf::RpcController*,
    const ::mir::protobuf::ConnectParameters* request,
    ::mir::protobuf::Connection* response,
    ::google::protobuf::Closure* done)
{
    report->session_connect_called(request->application_name());

    session = shell->open_session(request->application_name(), event_sink);

    auto ipc_package = graphics_platform->get_ipc_package();
    auto platform = response->mutable_platform();
    auto display_info = response->mutable_display_info();

    for (auto& data : ipc_package->ipc_data)
        platform->add_data(data);

    for (auto& ipc_fds : ipc_package->ipc_fds)
        platform->add_fd(ipc_fds);

    auto view_area = viewable_area->view_area();
    display_info->set_width(view_area.size.width.as_uint32_t());
    display_info->set_height(view_area.size.height.as_uint32_t());

    auto supported_pixel_formats = buffer_allocator->supported_pixel_formats();
    for (auto pf : supported_pixel_formats)
        display_info->add_supported_pixel_format(static_cast<uint32_t>(pf));

    resource_cache->save_resource(response, ipc_package);

    done->Run();
}

void mir::frontend::SessionMediator::create_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceParameters* request,
    mir::protobuf::Surface* response,
    google::protobuf::Closure* done)
{
    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_create_surface_called(session->name());

    auto const id = shell->create_surface_for(session,
        SurfaceCreationParameters()
        .of_name(request->surface_name())
        .of_size(request->width(), request->height())
        .of_buffer_usage(static_cast<compositor::BufferUsage>(request->buffer_usage()))
        .of_pixel_format(static_cast<geometry::PixelFormat>(request->pixel_format()))
        );

    {
        auto surface = session->get_surface(id);
        response->mutable_id()->set_value(id.as_value());
        response->set_width(surface->size().width.as_uint32_t());
        response->set_height(surface->size().height.as_uint32_t());
        response->set_pixel_format((int)surface->pixel_format());
        response->set_buffer_usage(request->buffer_usage());

        if (surface->supports_input())
            response->add_fd(surface->client_input_fd());

        auto const& buffer_resource = surface->client_buffer();

        auto const& id = buffer_resource->id();

        auto buffer = response->mutable_buffer();
        buffer->set_buffer_id(id.as_uint32_t());

        if (!client_tracker->client_has(id))
        {
            auto ipc_package = buffer_resource->get_ipc_package();

            for (auto& data : ipc_package->ipc_data)
                buffer->add_data(data);

            for (auto& ipc_fds : ipc_package->ipc_fds)
                buffer->add_fd(ipc_fds);

            buffer->set_stride(ipc_package->stride);

            resource_cache->save_resource(response, ipc_package);
        }
        client_tracker->add(id);
    }

    done->Run();
}

void mir::frontend::SessionMediator::next_buffer(
    ::google::protobuf::RpcController* /*controller*/,
    ::mir::protobuf::SurfaceId const* request,
    ::mir::protobuf::Buffer* response,
    ::google::protobuf::Closure* done)
{
    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_next_buffer_called(session->name());

    auto surface = session->get_surface(SurfaceId(request->value()));

    surface->advance_client_buffer();
    auto const& buffer_resource = surface->client_buffer();
    auto const& id = buffer_resource->id();
    response->set_buffer_id(id.as_uint32_t());

    if (!client_tracker->client_has(id))
    {
        auto ipc_package = buffer_resource->get_ipc_package();

        for (auto& data : ipc_package->ipc_data)
            response->add_data(data);

        for (auto& ipc_fds : ipc_package->ipc_fds)
            response->add_fd(ipc_fds);

        response->set_stride(ipc_package->stride);

        resource_cache->save_resource(response, ipc_package);
    }
    client_tracker->add(id);
    done->Run();
}

void mir::frontend::SessionMediator::release_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceId* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_release_surface_called(session->name());

    auto const id = SurfaceId(request->value());

    session->destroy_surface(id);

    done->Run();
}

void mir::frontend::SessionMediator::disconnect(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::Void* /*request*/,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_disconnect_called(session->name());

    shell->close_session(session);
    session.reset();

    done->Run();
}

void mir::frontend::SessionMediator::configure_surface(
    google::protobuf::RpcController*, // controller,
    const mir::protobuf::SurfaceSetting* request,
    mir::protobuf::SurfaceSetting* response,
    google::protobuf::Closure* done)
{
    MirSurfaceAttrib attrib = static_cast<MirSurfaceAttrib>(request->attrib());

    // Required response fields:
    response->mutable_surfaceid()->CopyFrom(request->surfaceid());
    response->set_attrib(attrib);

    if (session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

    report->session_configure_surface_called(session->name());

    auto const id = frontend::SurfaceId(request->surfaceid().value());
    int value = request->ivalue();
    int newvalue = session->configure_surface(id, attrib, value);

    response->set_ivalue(newvalue);

    done->Run();
}

