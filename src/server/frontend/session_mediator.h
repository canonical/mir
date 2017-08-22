/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "screencast_buffer_tracker.h"
#include "protobuf_ipc_factory.h"

#include "mir/extension_description.h"
#include "mir/frontend/connection_context.h"
#include "mir/frontend/surface_id.h"
#include "mir/frontend/buffer_stream_id.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/buffer_id.h"
#include "mir/protobuf/display_server_debug.h"
#include "mir_toolkit/common.h"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <map>

namespace mir
{
class Executor;

namespace cookie
{
class Authority;
}
namespace graphics
{
class Buffer;
class DisplayConfiguration;
class GraphicBufferAllocator;
}
namespace input
{
class CursorImages;
}

namespace scene
{
class CoordinateTranslator;
class ApplicationNotRespondingDetector;
}

/// Frontend interface. Mediates the interaction between client
/// processes and the core of the mir system.
namespace frontend
{
class Shell;
class Session;
class Surface;
class MessageResourceCache;
class SessionMediatorObserver;
class EventSink;
class EventSinkFactory;
class MessageSender;
class DisplayChanger;
class Screencast;
class PromptSession;
class BufferStream;
class InputConfigurationChanger;
class BufferMap;

namespace detail
{
typedef IntWrapper<struct PromptSessionTag> PromptSessionId;

struct PromptSessionStore
{
    auto insert(std::shared_ptr<PromptSession> const& session) -> PromptSessionId;
    auto fetch(PromptSessionId session) const -> std::shared_ptr<PromptSession>;
    void remove(PromptSessionId session);

private:
    std::mutex mutable mutex;
    int32_t next_id{0};
    std::map<PromptSessionId, std::weak_ptr<PromptSession>> mutable sessions;
};
}

/**
 * SessionMediator relays requests from the client process into the server.
 *
 * Each SessionMediator is associated with exactly one client socket connection, and
 * visa versa.
 *
 * \note SessionMediator is *not* reentrant. If two threads want to process events on a client
 *       socket at the same time they must perform their own locking.
 */
class SessionMediator : public detail::DisplayServer, public mir::protobuf::DisplayServerDebug
{
public:

    SessionMediator(
        std::shared_ptr<Shell> const& shell,
        std::shared_ptr<graphics::PlatformIpcOperations> const& ipc_operations,
        std::shared_ptr<frontend::DisplayChanger> const& display_changer,
        std::vector<MirPixelFormat> const& surface_pixel_formats,
        std::shared_ptr<SessionMediatorObserver> const& observer,
        std::shared_ptr<EventSinkFactory> const& sink_factory,
        std::shared_ptr<MessageSender> const& message_sender,
        std::shared_ptr<MessageResourceCache> const& resource_cache,
        std::shared_ptr<Screencast> const& screencast,
        ConnectionContext const& connection_context,
        std::shared_ptr<input::CursorImages> const& cursor_images,
        std::shared_ptr<scene::CoordinateTranslator> const& translator,
        std::shared_ptr<scene::ApplicationNotRespondingDetector> const& anr_detector,
        std::shared_ptr<cookie::Authority> const& cookie_authority,
        std::shared_ptr<InputConfigurationChanger> const& input_changer,
        std::vector<mir::ExtensionDescription> const& extensions,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        mir::Executor& executor);

    ~SessionMediator() noexcept;

    void client_pid(int pid) override;

    void connect(
        mir::protobuf::ConnectParameters const* request,
        mir::protobuf::Connection* response,
        google::protobuf::Closure* done) override;
    void disconnect(
        mir::protobuf::Void const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void create_surface(
        mir::protobuf::SurfaceParameters const* request,
        mir::protobuf::Surface* response,
        google::protobuf::Closure* done) override;
    void modify_surface(
        mir::protobuf::SurfaceModifications const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void release_surface(
        mir::protobuf::SurfaceId const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void platform_operation(
        mir::protobuf::PlatformOperationMessage const* request,
        mir::protobuf::PlatformOperationMessage* response,
        google::protobuf::Closure* done) override;
    void configure_surface(
        mir::protobuf::SurfaceSetting const* request,
        mir::protobuf::SurfaceSetting* response,
        google::protobuf::Closure* done) override;
    void configure_display(
        mir::protobuf::DisplayConfiguration const* request,
        mir::protobuf::DisplayConfiguration* response,
        google::protobuf::Closure* done) override;
    void remove_session_configuration(
        mir::protobuf::Void const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void set_base_display_configuration(
        mir::protobuf::DisplayConfiguration const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void preview_base_display_configuration(
        mir::protobuf::PreviewConfiguration const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void confirm_base_display_configuration(
        mir::protobuf::DisplayConfiguration const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void cancel_base_display_configuration_preview(
        mir::protobuf::Void const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void create_screencast(
        mir::protobuf::ScreencastParameters const* request,
        mir::protobuf::Screencast* response,
        google::protobuf::Closure* done) override;
    void screencast_buffer(
        mir::protobuf::ScreencastId const* request,
        mir::protobuf::Buffer* response,
        google::protobuf::Closure* done) override;
    void screencast_to_buffer(
        mir::protobuf::ScreencastRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void release_screencast(
        mir::protobuf::ScreencastId const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void create_buffer_stream(
        mir::protobuf::BufferStreamParameters const* request,
        mir::protobuf::BufferStream* response,
        google::protobuf::Closure* done) override;
    void release_buffer_stream(
        mir::protobuf::BufferStreamId const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void configure_cursor(
        mir::protobuf::CursorSetting const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void new_fds_for_prompt_providers(
        mir::protobuf::SocketFDRequest const* request,
        mir::protobuf::SocketFD* response,
        google::protobuf::Closure* done) override;
    void start_prompt_session(
        mir::protobuf::PromptSessionParameters const* request,
        mir::protobuf::PromptSession* response,
        google::protobuf::Closure* done) override;
    void stop_prompt_session(
        mir::protobuf::PromptSession const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void submit_buffer(
        mir::protobuf::BufferRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void allocate_buffers(
        mir::protobuf::BufferAllocation const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void release_buffers(
        mir::protobuf::BufferRelease const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void request_persistent_surface_id(
        mir::protobuf::SurfaceId const* request,
        mir::protobuf::PersistentSurfaceId* response,
        google::protobuf::Closure* done) override;
    void pong(
        mir::protobuf::PingEvent const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void configure_buffer_stream(
        mir::protobuf::StreamConfiguration const* request,
        mir::protobuf::Void*,
        google::protobuf::Closure* done) override;
    void request_operation(
        mir::protobuf::RequestWithAuthority const* request,
        mir::protobuf::Void*,
        google::protobuf::Closure* done) override;
    void apply_input_configuration(
        mir::protobuf::InputConfigurationRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;
    void set_base_input_configuration(
        mir::protobuf::InputConfigurationRequest const* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override;

    // TODO: Split this into a separate thing
    void translate_surface_to_screen(
        mir::protobuf::CoordinateTranslationRequest const* request,
        mir::protobuf::CoordinateTranslationResponse* response,
        google::protobuf::Closure* done) override;

private:
    void pack_protobuf_buffer(protobuf::Buffer& protobuf_buffer,
                              graphics::Buffer* graphics_buffer,
                              graphics::BufferIpcMsgType msg_type);

    std::shared_ptr<graphics::DisplayConfiguration> unpack_and_sanitize_display_configuration(
        protobuf::DisplayConfiguration const*);

    virtual std::function<void(std::shared_ptr<Session> const&)>
    prompt_session_connect_handler(detail::PromptSessionId prompt_session_id) const;

    void destroy_screencast_sessions();

    pid_t client_pid_;
    std::shared_ptr<Shell> const shell;
    std::shared_ptr<graphics::PlatformIpcOperations> const ipc_operations;

    std::vector<MirPixelFormat> const surface_pixel_formats;

    std::shared_ptr<frontend::DisplayChanger> const display_changer;
    std::shared_ptr<SessionMediatorObserver> const observer;
    std::shared_ptr<EventSinkFactory> const sink_factory;
    std::shared_ptr<EventSink> const event_sink;
    std::shared_ptr<MessageSender> const message_sender;
    std::shared_ptr<MessageResourceCache> const resource_cache;
    std::shared_ptr<Screencast> const screencast;
    ConnectionContext const connection_context;
    std::shared_ptr<input::CursorImages> const cursor_images;
    std::shared_ptr<scene::CoordinateTranslator> const translator;
    std::shared_ptr<scene::ApplicationNotRespondingDetector> const anr_detector;
    std::shared_ptr<cookie::Authority> const cookie_authority;
    std::shared_ptr<InputConfigurationChanger> const input_changer;
    std::vector<mir::ExtensionDescription> const extensions;
    std::unordered_map<graphics::BufferID, std::shared_ptr<graphics::Buffer>> buffer_cache;
    std::unordered_multimap<BufferStreamId, graphics::BufferID> stream_associated_buffers;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    mir::Executor& executor;

    ScreencastBufferTracker screencast_buffer_tracker;

    std::weak_ptr<Session> weak_session;
    detail::PromptSessionStore prompt_sessions;

    std::map<frontend::SurfaceId, frontend::BufferStreamId> legacy_default_stream_map;
};

}
}

#endif /* MIR_FRONTEND_SESSION_MEDIATOR_H_ */
