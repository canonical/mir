/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "display_server.h"
#include "mir/frontend/connection_context.h"
#include "mir/frontend/surface_id.h"
#include "mir/frontend/buffer_stream_id.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir_toolkit/common.h"
#include "buffer_stream_tracker.h"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace mir
{
namespace graphics
{
class Buffer;
class Display;
class GraphicBufferAllocator;
}
namespace input
{
class CursorImages;
}

namespace scene
{
class CoordinateTranslator;
}

/// Frontend interface. Mediates the interaction between client
/// processes and the core of the mir system.
namespace frontend
{
class ClientBufferTracker;
class Shell;
class Session;
class Surface;
class MessageResourceCache;
class SessionMediatorReport;
class EventSink;
class DisplayChanger;
class Screencast;
class PromptSession;
class BufferStream;

/**
 * SessionMediator relays requests from the client process into the server.
 *
 * Each SessionMediator is associated with exactly one client socket connection, and
 * visa versa.
 *
 * \note SessionMediator is *not* reentrant. If two threads want to process events on a client
 *       socket at the same time they must perform their own locking.
 */
class SessionMediator : public detail::DisplayServer, public mir::protobuf::Debug
{
public:

    SessionMediator(
        std::shared_ptr<Shell> const& shell,
        std::shared_ptr<graphics::PlatformIpcOperations> const& ipc_operations,
        std::shared_ptr<frontend::DisplayChanger> const& display_changer,
        std::vector<MirPixelFormat> const& surface_pixel_formats,
        std::shared_ptr<SessionMediatorReport> const& report,
        std::shared_ptr<EventSink> const& event_sink,
        std::shared_ptr<MessageResourceCache> const& resource_cache,
        std::shared_ptr<Screencast> const& screencast,
        ConnectionContext const& connection_context,
        std::shared_ptr<input::CursorImages> const& cursor_images,
        std::shared_ptr<scene::CoordinateTranslator> const& translator);

    ~SessionMediator() noexcept;

    void client_pid(int pid) override;

    /* Platform independent requests */
    void connect(::google::protobuf::RpcController* controller,
                 const ::mir::protobuf::ConnectParameters* request,
                 ::mir::protobuf::Connection* response,
                 ::google::protobuf::Closure* done) override;

    void create_surface(google::protobuf::RpcController* controller,
                        const mir::protobuf::SurfaceParameters* request,
                        mir::protobuf::Surface* response,
                        google::protobuf::Closure* done) override;

    void next_buffer(
        google::protobuf::RpcController* controller,
        mir::protobuf::SurfaceId const* request,
        mir::protobuf::Buffer* response,
        google::protobuf::Closure* done) override;

    void exchange_buffer(
        google::protobuf::RpcController* controller,
        mir::protobuf::BufferRequest const* request,
        mir::protobuf::Buffer* response,
        google::protobuf::Closure* done) override;

    void submit_buffer(
        google::protobuf::RpcController* controller,
        mir::protobuf::BufferRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;

    void allocate_buffers( 
        google::protobuf::RpcController* controller,
        mir::protobuf::BufferAllocation const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;

    void release_buffers(
        google::protobuf::RpcController* controller,
        mir::protobuf::BufferRelease const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;

    void release_surface(google::protobuf::RpcController* controller,
                         const mir::protobuf::SurfaceId*,
                         mir::protobuf::Void*,
                         google::protobuf::Closure* done) override;

    void disconnect(google::protobuf::RpcController* controller,
                    const mir::protobuf::Void* request,
                    mir::protobuf::Void* response,
                    google::protobuf::Closure* done) override;

    void configure_surface(google::protobuf::RpcController* controller,
                           const mir::protobuf::SurfaceSetting*,
                           mir::protobuf::SurfaceSetting*,
                           google::protobuf::Closure* done) override;

    void modify_surface(google::protobuf::RpcController*,
                        const mir::protobuf::SurfaceModifications*,
                        mir::protobuf::Void*,
                        google::protobuf::Closure*) override;

    void configure_display(::google::protobuf::RpcController* controller,
                           const ::mir::protobuf::DisplayConfiguration* request,
                           ::mir::protobuf::DisplayConfiguration* response,
                           ::google::protobuf::Closure* done) override;

    void create_screencast(google::protobuf::RpcController*,
                           const mir::protobuf::ScreencastParameters*,
                           mir::protobuf::Screencast*,
                           google::protobuf::Closure* done) override;

    void release_screencast(google::protobuf::RpcController*,
                            const mir::protobuf::ScreencastId*,
                            mir::protobuf::Void*,
                            google::protobuf::Closure* done) override;

    void screencast_buffer(google::protobuf::RpcController*,
                           const mir::protobuf::ScreencastId*,
                           mir::protobuf::Buffer*,
                           google::protobuf::Closure* done) override;

    void create_buffer_stream(google::protobuf::RpcController*,
                              mir::protobuf::BufferStreamParameters const*,
                              mir::protobuf::BufferStream*,
                              google::protobuf::Closure* done) override;
    void release_buffer_stream(google::protobuf::RpcController*,
                               mir::protobuf::BufferStreamId const*,
                               mir::protobuf::Void*,
                               google::protobuf::Closure* done) override;

    void configure_cursor(google::protobuf::RpcController*,
                          mir::protobuf::CursorSetting const*,
                          mir::protobuf::Void*,
                          google::protobuf::Closure* done) override;

    void start_prompt_session(::google::protobuf::RpcController* controller,
                            const ::mir::protobuf::PromptSessionParameters* request,
                            ::mir::protobuf::Void* response,
                            ::google::protobuf::Closure* done) override;

    void stop_prompt_session(::google::protobuf::RpcController* controller,
                            const ::mir::protobuf::Void* request,
                            ::mir::protobuf::Void* response,
                            ::google::protobuf::Closure* done) override;

    /* Platform specific requests */
    void drm_auth_magic(google::protobuf::RpcController* controller,
                        const mir::protobuf::DRMMagic* request,
                        mir::protobuf::DRMAuthMagicStatus* response,
                        google::protobuf::Closure* done) override;

    void platform_operation(
        google::protobuf::RpcController* /*controller*/,
        mir::protobuf::PlatformOperationMessage const* request,
        mir::protobuf::PlatformOperationMessage* response,
        google::protobuf::Closure* done) override;

    void new_fds_for_prompt_providers(
        ::google::protobuf::RpcController* controller,
        ::mir::protobuf::SocketFDRequest const* parameters,
        ::mir::protobuf::SocketFD* response,
        ::google::protobuf::Closure* done) override;

    // TODO: Split this into a separate thing
    void translate_surface_to_screen(
        ::google::protobuf::RpcController* controller,
        ::mir::protobuf::CoordinateTranslationRequest const* request,
        ::mir::protobuf::CoordinateTranslationResponse* response,
        ::google::protobuf::Closure *done) override;

    void request_persistent_surface_id(
        ::google::protobuf::RpcController* controller,
        ::mir::protobuf::SurfaceId const* request,
        ::mir::protobuf::PersistentSurfaceId* response,
        ::google::protobuf::Closure* done) override;

private:
    void pack_protobuf_buffer(protobuf::Buffer& protobuf_buffer,
                              graphics::Buffer* graphics_buffer,
                              graphics::BufferIpcMsgType msg_type);

    void advance_buffer(
        BufferStreamId surf_id,
        BufferStream& buffer_stream,
        graphics::Buffer* old_buffer,
        std::function<void(graphics::Buffer*, graphics::BufferIpcMsgType)> complete);

    virtual std::function<void(std::shared_ptr<Session> const&)> prompt_session_connect_handler() const;

    pid_t client_pid_;
    std::shared_ptr<Shell> const shell;
    std::shared_ptr<graphics::PlatformIpcOperations> const ipc_operations;

    std::vector<MirPixelFormat> const surface_pixel_formats;

    std::shared_ptr<frontend::DisplayChanger> const display_changer;
    std::shared_ptr<SessionMediatorReport> const report;
    std::shared_ptr<EventSink> const event_sink;
    std::shared_ptr<MessageResourceCache> const resource_cache;
    std::shared_ptr<Screencast> const screencast;
    ConnectionContext const connection_context;
    std::shared_ptr<input::CursorImages> const cursor_images;
    std::shared_ptr<scene::CoordinateTranslator> const translator;

    BufferStreamTracker buffer_stream_tracker;

    std::weak_ptr<Session> weak_session;
    std::weak_ptr<PromptSession> weak_prompt_session;
};

}
}

#endif /* MIR_FRONTEND_SESSION_MEDIATOR_H_ */
