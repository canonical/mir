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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_FRONTEND_APPLICATION_MEDIATOR_H_
#define MIR_FRONTEND_APPLICATION_MEDIATOR_H_

#include "mir_protobuf.pb.h"

#include <map>
#include <memory>

namespace mir
{
namespace graphics
{
class Platform;
class ViewableArea;
}

namespace compositor
{
class GraphicBufferAllocator;
}

namespace shell
{
class SessionStore;
class Session;
}

/// Frontend interface. Mediates the interaction between client
/// applications and the core of the mir system.
namespace frontend
{
class ResourceCache;
class ApplicationMediatorReport;

// ApplicationMediator relays requests from the client into the server process.
class ApplicationMediator : public mir::protobuf::DisplayServer
{
public:

    ApplicationMediator(
        std::shared_ptr<shell::SessionStore> const& session_store,
        std::shared_ptr<graphics::Platform> const& graphics_platform,
        std::shared_ptr<graphics::ViewableArea> const& viewable_area,
        std::shared_ptr<compositor::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<ApplicationMediatorReport> const& report,
        std::shared_ptr<ResourceCache> const& resource_cache);

    /* Platform independent requests */
    void connect(::google::protobuf::RpcController* controller,
                 const ::mir::protobuf::ConnectParameters* request,
                 ::mir::protobuf::Connection* response,
                 ::google::protobuf::Closure* done);

    void create_surface(google::protobuf::RpcController* controller,
                        const mir::protobuf::SurfaceParameters* request,
                        mir::protobuf::Surface* response,
                        google::protobuf::Closure* done);

    void next_buffer(
        google::protobuf::RpcController* controller,
        mir::protobuf::SurfaceId const* request,
        mir::protobuf::Buffer* response,
        google::protobuf::Closure* done);

    void select_focus_by_lightdm_id(
        google::protobuf::RpcController* controller,
        mir::protobuf::LightdmId const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done);

    void release_surface(google::protobuf::RpcController* controller,
                         const mir::protobuf::SurfaceId*,
                         mir::protobuf::Void*,
                         google::protobuf::Closure* done);

    void disconnect(google::protobuf::RpcController* controller,
                    const mir::protobuf::Void* request,
                    mir::protobuf::Void* response,
                    google::protobuf::Closure* done);

    /* Platform specific requests */
    void drm_auth_magic(google::protobuf::RpcController* controller,
                        const mir::protobuf::DRMMagic* request,
                        mir::protobuf::DRMAuthMagicStatus* response,
                        google::protobuf::Closure* done);

private:
    std::shared_ptr<shell::SessionStore> const session_store;
    std::shared_ptr<graphics::Platform> const graphics_platform;

    // TODO this is a dubious dependency - to get display_info (is there only one?)
    std::shared_ptr<graphics::ViewableArea> const viewable_area;
    // TODO this is a dubious dependency - to get supported_pixel_formats
    std::shared_ptr<compositor::GraphicBufferAllocator> const buffer_allocator;

    std::shared_ptr<ApplicationMediatorReport> const report;
    std::shared_ptr<ResourceCache> const resource_cache;

    std::shared_ptr<shell::Session> application_session;
};

}
}


#endif /* MIR_FRONTEND_APPLICATION_MEDIATOR_H_ */
