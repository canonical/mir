/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_MIR_CONNECTION_H_
#define MIR_CLIENT_MIR_CONNECTION_H_

#include "mir_wait_handle.h"
#include "lifecycle_control.h"
#include "ping_handler.h"
#include "rpc/mir_display_server.h"
#include "rpc/mir_display_server_debug.h"

#include "mir/extension_description.h"
#include "mir_toolkit/extensions/window_coordinate_translation.h"
#include "mir_toolkit/extensions/graphics_module.h"
#include "mir/geometry/size.h"
#include "mir/client/client_platform.h"
#include "mir/frontend/surface_id.h"
#include "mir/client/client_context.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_surface.h"
#include "display_configuration.h"
#include "error_handler.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace mir
{
namespace input
{
class InputDevices;
}
namespace protobuf
{
class BufferStream;
class Connection;
class ConnectParameters;
class PlatformOperationMessage;
class DisplayConfiguration;
}
/// The client-side library implementation namespace
namespace client
{
class ConnectionConfiguration;
class ClientPlatformFactory;
class ConnectionSurfaceMap;
class DisplayConfiguration;
class EventHandlerRegister;
class AsyncBufferFactory;
class MirBuffer;
class BufferStream;
class PresentationChain;

namespace rpc
{
class DisplayServer;
class DisplayServerDebug;
class MirBasicRpcChannel;
}
}

namespace logging
{
class Logger;
}

namespace dispatch
{
class ThreadedDispatcher;
}
}

struct MirConnection : mir::client::ClientContext
{
public:
    MirConnection(std::string const& error_message);

    MirConnection(mir::client::ConnectionConfiguration& conf);
    ~MirConnection() noexcept;

    MirConnection(MirConnection const &) = delete;
    MirConnection& operator=(MirConnection const &) = delete;

    MirWaitHandle* create_surface(
        MirWindowSpec const& spec,
        MirWindowCallback callback,
        void * context);
    MirWaitHandle* release_surface(
        MirWindow *surface,
        MirWindowCallback callback,
        void *context);

    MirPromptSession* create_prompt_session();

    char const * get_error_message();

    MirWaitHandle* connect(
        const char* app_name,
        MirConnectedCallback callback,
        void * context);

    MirWaitHandle* disconnect();

    MirWaitHandle* platform_operation(
        MirPlatformMessage const* request,
        MirPlatformOperationCallback callback, void* context) override;

    void register_lifecycle_event_callback(MirLifecycleEventCallback callback, void* context);

    void register_ping_event_callback(MirPingEventCallback callback, void* context);
    void pong(int32_t serial);

    void register_display_change_callback(MirDisplayConfigCallback callback, void* context);

    void register_error_callback(MirErrorCallback callback, void* context);

    void populate(MirPlatformPackage& platform_package);
    void populate_graphics_module(MirModuleProperties& properties) override;
    MirDisplayConfiguration* create_copy_of_display_config();
    std::unique_ptr<mir::protobuf::DisplayConfiguration> snapshot_display_configuration() const;
    void available_surface_formats(MirPixelFormat* formats,
                                   unsigned int formats_size, unsigned int& valid_formats);

    std::shared_ptr<MirBufferStream> make_consumer_stream(
       mir::protobuf::BufferStream const& protobuf_bs);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirWaitHandle* create_client_buffer_stream(
        int width, int height,
        MirPixelFormat format,
        MirBufferUsage buffer_usage,
        MirRenderSurface* render_surface,
        MirBufferStreamCallback mbs_callback,
        void *context);
    std::shared_ptr<mir::client::BufferStream> create_client_buffer_stream_with_id(
        int width, int height,
        MirRenderSurface* render_surface,
        mir::protobuf::BufferStream const& a_protobuf_bs);
    MirWaitHandle* release_buffer_stream(
        MirBufferStream*,
        MirBufferStreamCallback callback,
        void *context);

    std::shared_ptr<mir::client::PresentationChain> create_presentation_chain_with_id(
        MirRenderSurface* render_surface,
        mir::protobuf::BufferStream const& a_protobuf_bs);

    void release_consumer_stream(MirBufferStream*);

    static bool is_valid(MirConnection *connection);

    EGLNativeDisplayType egl_native_display();
    MirPixelFormat       egl_pixel_format(EGLDisplay, EGLConfig) const;

    void on_stream_created(int id, MirBufferStream* stream);

    MirWaitHandle* configure_display(MirDisplayConfiguration* configuration);
    void done_display_configure();

    void configure_session_display(mir::protobuf::DisplayConfiguration const& configuration);
    void remove_session_display();

    MirWaitHandle* set_base_display_configuration(MirDisplayConfiguration const* configuration);
    void apply_input_configuration(MirInputConfig  const* config);
    void set_base_input_configuration(MirInputConfig const* config);
    void preview_base_display_configuration(
        mir::protobuf::DisplayConfiguration const& configuration,
        std::chrono::seconds timeout);
    void confirm_base_display_configuration(
        mir::protobuf::DisplayConfiguration const& configuration);
    void cancel_base_display_configuration_preview();
    void done_set_base_display_configuration();

    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> rpc_channel() const
    {
        return channel;
    }

    mir::client::rpc::DisplayServer& display_server();
    mir::client::rpc::DisplayServerDebug& debug_display_server();
    std::shared_ptr<mir::input::InputDevices> const& the_input_devices() const
    {
        return input_devices;
    }

    std::shared_ptr<mir::client::ConnectionSurfaceMap> const& connection_surface_map() const
    {
        return surface_map;
    }

    void allocate_buffer(
        mir::geometry::Size size, MirPixelFormat format,
        MirBufferCallback callback, void* context) override;
    void allocate_buffer(
        mir::geometry::Size size, uint32_t native_format, uint32_t native_flags,
        MirBufferCallback callback, void* context) override;
    void release_buffer(mir::client::MirBuffer* buffer) override;

    auto create_render_surface_with_content(
        mir::geometry::Size logical_size,
        MirRenderSurfaceCallback callback,
        void* context) -> MirRenderSurface*;
#pragma GCC diagnostic pop
    void release_render_surface_with_content(
        void* render_surface);

    void* request_interface(char const* name, int version);

    void enumerate_extensions(
        void* context,
        void (*enumerator)(void* context, char const* extension, int version));

private:
    //google cant have callbacks with more than 2 args
    struct SurfaceCreationRequest
    {
        SurfaceCreationRequest(MirWindowCallback cb, void* context, MirWindowSpec const& spec) :
            cb(cb), context(context), spec(spec),
              response(std::make_shared<mir::protobuf::Surface>()),
              wh(std::make_shared<MirWaitHandle>())
        {
        }
        MirWindowCallback cb;
        void* context;
        MirWindowSpec const spec;
        std::shared_ptr<mir::protobuf::Surface> response;
        std::shared_ptr<MirWaitHandle> wh;
    };
    std::vector<std::shared_ptr<SurfaceCreationRequest>> surface_requests;
    void surface_created(SurfaceCreationRequest*);

    struct StreamCreationRequest
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        StreamCreationRequest(
            MirRenderSurface* rs,
            MirBufferStreamCallback mbs_cb,
            void* context,
            mir::protobuf::BufferStreamParameters const& params)
            : rs(rs),
              mbs_callback(mbs_cb),
              context(context),
              parameters(params),
              response(std::make_shared<mir::protobuf::BufferStream>()),
              wh(std::make_shared<MirWaitHandle>())
        {
        }
        MirRenderSurface* rs;
#pragma GCC diagnostic pop
        MirBufferStreamCallback mbs_callback;
        void* context;
        mir::protobuf::BufferStreamParameters const parameters;
        std::shared_ptr<mir::protobuf::BufferStream> response;
        std::shared_ptr<MirWaitHandle> const wh;
    };
    std::vector<std::shared_ptr<StreamCreationRequest>> stream_requests;
    void stream_created(StreamCreationRequest*);
    void stream_error(std::string const& error_msg, std::shared_ptr<StreamCreationRequest> const& request);

    struct RenderSurfaceCreationRequest
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        RenderSurfaceCreationRequest(
            MirRenderSurfaceCallback cb,
            void* context,
            std::shared_ptr<void> native_window,
            mir::geometry::Size size) :
                callback(cb), context(context),
                response(std::make_shared<mir::protobuf::BufferStream>()),
                wh(std::make_shared<MirWaitHandle>()),
                native_window(native_window),
                logical_size(size)
        {
        }

        MirRenderSurfaceCallback callback;
#pragma GCC diagnostic pop
        void* context;
        std::shared_ptr<mir::protobuf::BufferStream> response;
        std::shared_ptr<MirWaitHandle> const wh;
        std::shared_ptr<void> native_window;
        mir::geometry::Size logical_size;
    };

    std::vector<std::shared_ptr<RenderSurfaceCreationRequest>> render_surface_requests;
    void render_surface_created(RenderSurfaceCreationRequest*);
    void render_surface_error(std::string const& error_msg, std::shared_ptr<RenderSurfaceCreationRequest> const& request);

    void populate_server_package(MirPlatformPackage& platform_package) override;
    // MUST be first data member so it is destroyed last.
    struct Deregisterer
    { MirConnection* const self; ~Deregisterer(); } deregisterer;

    mutable std::mutex mutex; // Protects all members of *this (except release_wait_handles)

    std::shared_ptr<mir::client::ClientPlatform> platform;
    std::shared_ptr<mir::client::ConnectionSurfaceMap> surface_map;
    std::shared_ptr<mir::client::AsyncBufferFactory> buffer_factory;
    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> const channel;
    mir::client::rpc::DisplayServer server;
    mir::client::rpc::DisplayServerDebug debug;
    std::shared_ptr<mir::logging::Logger> const logger;
    std::unique_ptr<mir::protobuf::Void> void_response;
    std::unique_ptr<mir::protobuf::Connection> connect_result;
    std::atomic<bool> connect_done;
    std::unique_ptr<mir::protobuf::Void> ignored;
    std::unique_ptr<mir::protobuf::ConnectParameters> connect_parameters;
    std::unique_ptr<mir::protobuf::PlatformOperationMessage> platform_operation_reply;
    std::unique_ptr<mir::protobuf::DisplayConfiguration> display_configuration_response;
    std::unique_ptr<mir::protobuf::Void> set_base_display_configuration_response;
    std::atomic<bool> disconnecting{false};

    mir::frontend::SurfaceId next_error_id(std::unique_lock<std::mutex> const&);
    int surface_error_id{-1};

    std::shared_ptr<mir::client::ClientPlatformFactory> const client_platform_factory;
    std::shared_ptr<mir::client::ClientBufferFactory> client_buffer_factory;
    std::shared_ptr<EGLNativeDisplayType> native_display;

    std::string error_message;

    MirWaitHandle connect_wait_handle;
    MirWaitHandle disconnect_wait_handle;
    MirWaitHandle platform_operation_wait_handle;
    MirWaitHandle configure_display_wait_handle;
    MirWaitHandle set_base_display_configuration_wait_handle;

    std::mutex release_wait_handle_guard;
    std::vector<std::unique_ptr<MirWaitHandle>> release_wait_handles;

    std::shared_ptr<mir::client::DisplayConfiguration> const display_configuration;
    std::shared_ptr<mir::input::InputDevices> const input_devices;

    std::shared_ptr<mir::client::LifecycleControl> const lifecycle_control;

    std::shared_ptr<mir::client::PingHandler> const ping_handler;

    std::shared_ptr<mir::client::ErrorHandler> error_handler;

    std::shared_ptr<mir::client::EventHandlerRegister> const event_handler_register;

    std::unique_ptr<google::protobuf::Closure> const pong_callback;

    std::unique_ptr<mir::dispatch::ThreadedDispatcher> const eventloop;
    

    struct SurfaceRelease;
    struct StreamRelease;

    MirConnection* next_valid{nullptr};

    void set_error_message(std::string const& error);
    void done_disconnect();
    void connected(MirConnectedCallback callback, void * context);
    void released(SurfaceRelease);
    void released(StreamRelease);
    void done_platform_operation(MirPlatformOperationCallback, void* context);
    bool validate_user_display_config(MirDisplayConfiguration const* config);

    int const nbuffers;
    mir::optional_value<MirExtensionWindowCoordinateTranslationV1> translation_ext;
    mir::optional_value<MirExtensionGraphicsModuleV1> graphics_module_extension;

    std::vector<mir::ExtensionDescription> extensions;
};

#endif /* MIR_CLIENT_MIR_CONNECTION_H_ */
