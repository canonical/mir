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

#include "mir/thread/all.h"
#include "mir_protobuf.pb.h"

#include <map>
#include <memory>

namespace mir
{
namespace surfaces
{
class ApplicationSurfaceOrganiser;
class Surface;
}
namespace frontend
{

class ApplicationProxy : public mir::protobuf::DisplayServer
{
public:

    ApplicationProxy(std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> const& surface_organiser);

    std::string const& name() const { return app_name; }

private:
    virtual void connect(::google::protobuf::RpcController* controller,
                         const ::mir::protobuf::ConnectParameters* request,
                         ::mir::protobuf::Void* response,
                         ::google::protobuf::Closure* done);

    void create_surface(google::protobuf::RpcController* controller,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done);

    void release_surface(google::protobuf::RpcController* controller,
                         const mir::protobuf::SurfaceId*,
                         mir::protobuf::Void*,
                         google::protobuf::Closure* done);

    void disconnect(google::protobuf::RpcController* controller,
                 const mir::protobuf::Void* request,
                 mir::protobuf::Void* response,
                 google::protobuf::Closure* done);

    void test_file_descriptors(
        google::protobuf::RpcController* controller,
        const mir::protobuf::Void* request,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done);

    int next_id();

    std::string app_name;
    std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> surface_organiser;
    std::atomic<int> next_surface_id;

    typedef std::map<int, std::weak_ptr<surfaces::Surface>> Surfaces;
    Surfaces surfaces;
};

}
}


#endif /* MIR_FRONTEND_APPLICATION_PROXY_H_ */
