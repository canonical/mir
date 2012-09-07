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

#include "mir/geometry/dimensions.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/surfaces/surface.h"




mir::frontend::ApplicationProxy::ApplicationProxy(
    std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> const& surface_organiser) :
    surface_organiser(surface_organiser), next_surface_id(0)
{
}

void mir::frontend::ApplicationProxy::create_surface(google::protobuf::RpcController* /*controller*/,
             const mir::protobuf::SurfaceParameters* request,
             mir::protobuf::Surface* response,
             google::protobuf::Closure* done)
{
    auto tmp = surface_organiser->create_surface(
        surfaces::SurfaceCreationParameters()
        .of_width(geometry::Width(request->width()))
        .of_height(geometry::Height(request->height()))
        );

    auto surface = tmp.lock();
    response->mutable_id()->set_value(next_surface_id++);
    response->set_width(surface->width().as_uint32_t());
    response->set_height(surface->height().as_uint32_t());
    response->set_pixel_format((int)surface->pixel_format());

    // TODO track server-side surface object
    done->Run();
}

void mir::frontend::ApplicationProxy::release_surface(google::protobuf::RpcController* /*controller*/,
                     const mir::protobuf::SurfaceId*,
                     mir::protobuf::Void*,
                     google::protobuf::Closure* done)
{
    // TODO implement this
    done->Run();
}

void mir::frontend::ApplicationProxy::disconnect(google::protobuf::RpcController* /*controller*/,
             const mir::protobuf::Void* /*request*/,
             mir::protobuf::Void* /*response*/,
             google::protobuf::Closure* done)
{
    // TODO implement this
    done->Run();
}
