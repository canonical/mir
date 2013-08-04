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


#ifndef MIR_FRONTEND_SESSION_MEDIATOR_H_
#define MIR_FRONTEND_SESSION_MEDIATOR_H_

#include "mir_protobuf.pb.h"

#include <map>
#include <memory>
#include <mutex>

namespace mir
{
namespace shell
{
class DisplayChanger;
}
namespace graphics
{
class Buffer;
class Platform;
class Display;
class GraphicBufferAllocator;
}


/// Frontend interface. Mediates the interaction between client
/// processes and the core of the mir system.
namespace frontend
{
class Shell;
class Session;
class ResourceCache;
class SessionMediatorReport;
class ClientBufferTracker;
class EventSink;

// SessionMediator relays requests from the client process into the server.
class SessionMediator : public mir::protobuf::DisplayServer
{
public:
    SessionMediator(
        std::shared_ptr<Shell> const& shell,
        std::shared_ptr<graphics::Platform> const& graphics_platform,
        std::shared_ptr<shell::DisplayChanger> const& display_changer,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<SessionMediatorReport> const& report,
        std::shared_ptr<EventSink> const& event_sink,
        std::shared_ptr<ResourceCache> const& resource_cache);

    ~SessionMediator() noexcept;

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

    void release_surface(google::protobuf::RpcController* controller,
                         const mir::protobuf::SurfaceId*,
                         mir::protobuf::Void*,
                         google::protobuf::Closure* done);

    void disconnect(google::protobuf::RpcController* controller,
                    const mir::protobuf::Void* request,
                    mir::protobuf::Void* response,
                    google::protobuf::Closure* done);

    void configure_surface(google::protobuf::RpcController* controller,
                           const mir::protobuf::SurfaceSetting*,
                           mir::protobuf::SurfaceSetting*,
                           google::protobuf::Closure* done);

    void configure_display(::google::protobuf::RpcController* controller,
                       const ::mir::protobuf::DisplayConfiguration* request,
                       ::mir::protobuf::Void* response,
                       ::google::protobuf::Closure* done);

    /* Platform specific requests */
    void drm_auth_magic(google::protobuf::RpcController* controller,
                        const mir::protobuf::DRMMagic* request,
                        mir::protobuf::DRMAuthMagicStatus* response,
                        google::protobuf::Closure* done);

private:
    std::shared_ptr<Shell> const shell;
    std::shared_ptr<graphics::Platform> const graphics_platform;

    // TODO this is a dubious dependency - to get supported_pixel_formats
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;

    std::shared_ptr<shell::DisplayChanger> const display_changer;
    std::shared_ptr<SessionMediatorReport> const report;
    std::shared_ptr<EventSink> const event_sink;
    std::shared_ptr<ResourceCache> const resource_cache;
    std::shared_ptr<ClientBufferTracker> const client_tracker;

    std::shared_ptr<graphics::Buffer> client_buffer_resource;

    std::mutex session_mutex;
    std::shared_ptr<Session> session;
};

}
}


#endif /* MIR_FRONTEND_SESSION_MEDIATOR_H_ */
