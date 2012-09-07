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


#ifndef MIR_FRONTEND_APPLICATION_PROXY_H_
#define MIR_FRONTEND_APPLICATION_PROXY_H_

#include "mir/geometry/dimensions.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/surfaces/surface.h"

#include "mir_protobuf.pb.h"

namespace mir
{
namespace frontend
{

class ApplicationProxy : public mir::protobuf::DisplayServer
{
public:

    ApplicationProxy(std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> const& surface_organiser) :
        surface_organiser(surface_organiser), next_surface_id(0)
    {
    }

private:
    void create_surface(google::protobuf::RpcController* /*controller*/,
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

    void release_surface(google::protobuf::RpcController* /*controller*/,
                         const mir::protobuf::SurfaceId*,
                         mir::protobuf::Void*,
                         google::protobuf::Closure* done)
    {
        // TODO implement this
        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        // TODO implement this
        done->Run();
    }

    std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> surface_organiser;
    int next_surface_id;
};

}
}


#endif /* MIR_FRONTEND_APPLICATION_PROXY_H_ */
