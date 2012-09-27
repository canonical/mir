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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/application_proxy.h"

#include "mir/compositor/buffer_ipc_package.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/surfaces/surface.h"


mir::frontend::ApplicationProxy::ApplicationProxy(
    std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> const& surface_organiser,
    std::shared_ptr<graphics::Platform> const & graphics_platform) :
    surface_organiser(surface_organiser),
    graphics_platform(graphics_platform),
    next_surface_id(0)
{
}

void mir::frontend::ApplicationProxy::connect(
    ::google::protobuf::RpcController*,
                     const ::mir::protobuf::ConnectParameters* request,
                     ::mir::protobuf::Connection* response,
                     ::google::protobuf::Closure* done)
{
    app_name = request->application_name();
    auto ipc_package = graphics_platform->get_ipc_package();

    for (auto p = ipc_package->ipc_data.begin(); p != ipc_package->ipc_data.end(); ++p)
        response->add_data(*p);

    for (auto p = ipc_package->ipc_fds.begin(); p != ipc_package->ipc_fds.end(); ++p)
        response->add_fd(*p);

    done->Run();
}

void mir::frontend::ApplicationProxy::create_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceParameters* request,
    mir::protobuf::Surface* response,
    google::protobuf::Closure* done)
{
    auto handle = surface_organiser->create_surface(
        surfaces::SurfaceCreationParameters()
        .of_name(request->surface_name())
        .of_size(geometry::Size{geometry::Width{request->width()},
                                geometry::Height{request->height()}})
        );

    auto const id = next_id();
    {
        auto surface = handle.lock();
        response->mutable_id()->set_value(id);
        response->set_width(surface->size().width.as_uint32_t());
        response->set_height(surface->size().height.as_uint32_t());
        response->set_pixel_format((int)surface->pixel_format());

        auto const& ipc_package = surface->get_buffer_ipc_package();
        auto buffer = response->mutable_buffer();

        for (auto p = ipc_package->ipc_data.begin(); p != ipc_package->ipc_data.end(); ++p)
            buffer->add_data(*p);

        for (auto p = ipc_package->ipc_fds.begin(); p != ipc_package->ipc_fds.end(); ++p)
            buffer->add_fd(*p);
    }

    surfaces[id] = handle;

    done->Run();
}

void mir::frontend::ApplicationProxy::next_buffer(
    ::google::protobuf::RpcController* /*controller*/,
    ::mir::protobuf::SurfaceId const* request,
    ::mir::protobuf::Buffer* response,
    ::google::protobuf::Closure* done)
{
    auto surface = surfaces[request->value()].lock();

    auto const& ipc_package = surface->get_buffer_ipc_package();

    for (auto p = ipc_package->ipc_data.begin(); p != ipc_package->ipc_data.end(); ++p)
        response->add_data(*p);

    for (auto p = ipc_package->ipc_fds.begin(); p != ipc_package->ipc_fds.end(); ++p)
        response->add_fd(*p);

    done->Run();
}


int mir::frontend::ApplicationProxy::next_id()
{
    int id = next_surface_id.load();
    while (!next_surface_id.compare_exchange_weak(id, id + 1));
    return id;
}

void mir::frontend::ApplicationProxy::release_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceId* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    auto const id = request->value();

    auto p = surfaces.find(id);

    if (p != surfaces.end())
    {
        surface_organiser->destroy_surface(p->second);
        surfaces.erase(p);
    }
    else
    {
        //TODO proper error logging
        // std::cerr << "ERROR trying to destroy unknown surface" << std::endl;
    }

    done->Run();
}

void mir::frontend::ApplicationProxy::disconnect(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::Void* /*request*/,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    for (auto p = surfaces.begin(); p != surfaces.end(); ++p)
    {
        surface_organiser->destroy_surface(p->second);
    }

    done->Run();
}

void mir::frontend::ApplicationProxy::test_file_descriptors(
    ::google::protobuf::RpcController*,
    const ::mir::protobuf::Void*,
    ::mir::protobuf::Buffer*,
    ::google::protobuf::Closure* done)
{
  // TODO

  done->Run();
}
