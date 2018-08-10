/*
 * Copyright © 2012-2016 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_connection.h"
#include "drag_and_drop.h"
#include "mir_surface.h"
#include "mir_prompt_session.h"
#include "mir_toolkit/extensions/graphics_module.h"
#include "mir_protobuf.pb.h"
#include "make_protobuf_object.h"
#include "mir_toolkit/mir_platform_message.h"
#include "mir/client/client_platform.h"
#include "mir/client/client_platform_factory.h"
#include "rpc/mir_basic_rpc_channel.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/input/input_devices.h"
#include "connection_configuration.h"
#include "display_configuration.h"
#include "connection_surface_map.h"
#include "lifecycle_control.h"
#include "error_stream.h"
#include "buffer_stream.h"
#include "screencast_stream.h"
#include "perf_report.h"
#include "render_surface.h"
#include "error_render_surface.h"
#include "presentation_chain.h"
#include "logging/perf_report.h"
#include "lttng/perf_report.h"
#include "buffer_factory.h"
#include "mir/require.h"
#include "mir/uncaught.h"

#include "mir/input/mir_input_config.h"
#include "mir/input/mir_input_config_serialization.h"
#include "mir/events/event_builders.h"
#include "mir/logging/logger.h"
#include "mir/platform_message.h"
#include "mir_error.h"

#include <algorithm>
#include <cstddef>
#include <unistd.h>
#include <signal.h>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace md = mir::dispatch;
namespace mircv = mir::input::receiver;
namespace mev = mir::events;
namespace gp = google::protobuf;
namespace mf = mir::frontend;
namespace mp = mir::protobuf;
namespace mi = mir::input;
namespace ml = mir::logging;
namespace geom = mir::geometry;

namespace
{
void ignore()
{
}

std::shared_ptr<mcl::PerfReport>
make_perf_report(std::shared_ptr<ml::Logger> const& logger)
{
    // TODO: It seems strange that this directly uses getenv
    const char* report_target = getenv("MIR_CLIENT_PERF_REPORT");
    if (report_target && !strcmp(report_target, "log"))
    {
        return std::make_shared<mcl::logging::PerfReport>(logger);
    }
    else if (report_target && !strcmp(report_target, "lttng"))
    {
        return std::make_shared<mcl::lttng::PerfReport>();
    }
    else
    {
        return std::make_shared<mcl::NullPerfReport>();
    }
}

size_t get_nbuffers_from_env()
{
    //TODO: (kdub) cannot set nbuffers == 2 in production until we have client-side
    //      timeout-based overallocation for timeout-based framedropping.
    const char* nbuffers_opt = getenv("MIR_CLIENT_NBUFFERS");
    if (nbuffers_opt && !strcmp(nbuffers_opt, "2"))
        return 2u;
    return 3u;
}

struct OnScopeExit
{
    ~OnScopeExit() { f(); }
    std::function<void()> const f;
};

mir::protobuf::SurfaceParameters serialize_spec(MirWindowSpec const& spec)
{
    mp::SurfaceParameters message;

#define SERIALIZE_OPTION_IF_SET(option) \
    if (spec.option.is_set()) \
        message.set_##option(spec.option.value());

    SERIALIZE_OPTION_IF_SET(width);
    SERIALIZE_OPTION_IF_SET(height);
    SERIALIZE_OPTION_IF_SET(pixel_format);
    SERIALIZE_OPTION_IF_SET(buffer_usage);
    SERIALIZE_OPTION_IF_SET(surface_name);
    SERIALIZE_OPTION_IF_SET(output_id);
    SERIALIZE_OPTION_IF_SET(type);
    SERIALIZE_OPTION_IF_SET(state);
    SERIALIZE_OPTION_IF_SET(pref_orientation);
    SERIALIZE_OPTION_IF_SET(edge_attachment);
    SERIALIZE_OPTION_IF_SET(placement_hints);
    SERIALIZE_OPTION_IF_SET(surface_placement_gravity);
    SERIALIZE_OPTION_IF_SET(aux_rect_placement_gravity);
    SERIALIZE_OPTION_IF_SET(aux_rect_placement_offset_x);
    SERIALIZE_OPTION_IF_SET(aux_rect_placement_offset_y);
    SERIALIZE_OPTION_IF_SET(min_width);
    SERIALIZE_OPTION_IF_SET(min_height);
    SERIALIZE_OPTION_IF_SET(max_width);
    SERIALIZE_OPTION_IF_SET(max_height);
    SERIALIZE_OPTION_IF_SET(width_inc);
    SERIALIZE_OPTION_IF_SET(height_inc);
    SERIALIZE_OPTION_IF_SET(shell_chrome);
    SERIALIZE_OPTION_IF_SET(confine_pointer);
    // min_aspect is a special case (below)
    // max_aspect is a special case (below)

    if (spec.parent.is_set() && spec.parent.value() != nullptr)
        message.set_parent_id(spec.parent.value()->id());

    if (spec.parent_id)
    {
        auto id = message.mutable_parent_persistent_id();
        id->set_value(spec.parent_id->as_string());
    }

    if (spec.aux_rect.is_set())
    {
        message.mutable_aux_rect()->set_left(spec.aux_rect.value().left);
        message.mutable_aux_rect()->set_top(spec.aux_rect.value().top);
        message.mutable_aux_rect()->set_width(spec.aux_rect.value().width);
        message.mutable_aux_rect()->set_height(spec.aux_rect.value().height);
    }

    if (spec.min_aspect.is_set())
    {
        message.mutable_min_aspect()->set_width(spec.min_aspect.value().width);
        message.mutable_min_aspect()->set_height(spec.min_aspect.value().height);
    }

    if (spec.max_aspect.is_set())
    {
        message.mutable_max_aspect()->set_width(spec.max_aspect.value().width);
        message.mutable_max_aspect()->set_height(spec.max_aspect.value().height);
    }

    if (spec.input_shape.is_set())
    {
        for (auto const& rect : spec.input_shape.value())
        {
            auto const new_shape = message.add_input_shape();
            new_shape->set_left(rect.left);
            new_shape->set_top(rect.top);
            new_shape->set_width(rect.width);
            new_shape->set_height(rect.height);
        }
    }

    if (spec.streams.is_set())
    {
        for(auto const& stream : spec.streams.value())
        {
            auto const new_stream = message.add_stream();
            new_stream->set_displacement_x(stream.displacement.dx.as_int());
            new_stream->set_displacement_y(stream.displacement.dy.as_int());
            new_stream->mutable_id()->set_value(stream.stream_id);
            if (stream.size.is_set())
            {
                new_stream->set_width(stream.size.value().width.as_int());
                new_stream->set_height(stream.size.value().height.as_int());
            }
        }
    }

    return message;
}

class MirDisplayConfigurationStore
{
public:
    MirDisplayConfigurationStore(MirDisplayConfiguration* config)
        : config{config}
    {
    }

    ~MirDisplayConfigurationStore()
    {
        mcl::delete_config_storage(config);
    }

    MirDisplayConfiguration* operator->() const { return config; }

private:
    MirDisplayConfigurationStore(MirDisplayConfigurationStore const&) = delete;
    MirDisplayConfigurationStore& operator=(MirDisplayConfigurationStore const&) = delete;

    MirDisplayConfiguration* const config;
};

void populate_protobuf_display_configuration(
    mp::DisplayConfiguration& protobuf_config,
    MirDisplayConfiguration const* config)
{
    for (auto i = 0u; i < config->num_outputs; i++)
    {
        auto const& output = config->outputs[i];
        auto protobuf_output = protobuf_config.add_display_output();
        protobuf_output->set_output_id(output.output_id);
        protobuf_output->set_used(output.used);
        protobuf_output->set_current_mode(output.current_mode);
        protobuf_output->set_current_format(output.current_format);
        protobuf_output->set_position_x(output.position_x);
        protobuf_output->set_position_y(output.position_y);
        protobuf_output->set_power_mode(output.power_mode);
        protobuf_output->set_orientation(output.orientation);
    }
}

void translate_coordinates(
    MirWindow* window,
    int x, int y,
    int* screen_x, int* screen_y)
try
{
    mir::require(window && screen_x && screen_y);
    window->translate_to_screen_coordinates(x, y, screen_x, screen_y);
}
catch (std::exception& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    *screen_x = 0;
    *screen_y = 0;
}

void get_graphics_module(MirConnection *connection, MirModuleProperties *properties)
try
{
    mir::require(connection && properties);
    connection->populate_graphics_module(*properties);
}
catch (std::exception& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

std::mutex connection_guard;
MirConnection* valid_connections{nullptr};
}

MirConnection::Deregisterer::~Deregisterer()
{
    std::lock_guard<std::mutex> lock(connection_guard);

    for (auto current = &valid_connections; *current; current = &(*current)->next_valid)
    {
        if (self == *current)
        {
            *current = self->next_valid;
            break;
        }
    }
}

mcl::ClientContext* mcl::to_client_context(MirConnection* connection)
{
    return connection;
}

MirConnection::MirConnection(std::string const& error_message) :
    deregisterer{this},
    channel(),
    server(nullptr),
    debug(nullptr),
    error_message(error_message),
    nbuffers(get_nbuffers_from_env())
{
}

MirConnection::MirConnection(
    mir::client::ConnectionConfiguration& conf) :
        deregisterer{this},
        surface_map(conf.the_surface_map()),
        buffer_factory(conf.the_buffer_factory()),
        channel(conf.the_rpc_channel()),
        server(channel),
        debug(channel),
        logger(conf.the_logger()),
        void_response{mcl::make_protobuf_object<mir::protobuf::Void>()},
        connect_result{mcl::make_protobuf_object<mir::protobuf::Connection>()},
        connect_done{false},
        ignored{mcl::make_protobuf_object<mir::protobuf::Void>()},
        connect_parameters{mcl::make_protobuf_object<mir::protobuf::ConnectParameters>()},
        platform_operation_reply{mcl::make_protobuf_object<mir::protobuf::PlatformOperationMessage>()},
        display_configuration_response{mcl::make_protobuf_object<mir::protobuf::DisplayConfiguration>()},
        set_base_display_configuration_response{mcl::make_protobuf_object<mir::protobuf::Void>()},
        client_platform_factory(conf.the_client_platform_factory()),
        display_configuration(conf.the_display_configuration()),
        input_devices{conf.the_input_devices()},
        lifecycle_control(conf.the_lifecycle_control()),
        ping_handler{conf.the_ping_handler()},
        error_handler{conf.the_error_handler()},
        event_handler_register(conf.the_event_handler_register()),
        pong_callback(google::protobuf::NewPermanentCallback(&google::protobuf::DoNothing)),
        eventloop{new md::ThreadedDispatcher{"RPC Thread", std::dynamic_pointer_cast<md::Dispatchable>(channel)}},
        nbuffers(get_nbuffers_from_env())
{
    connect_result->set_error("connect not called");
    {
        std::lock_guard<std::mutex> lock(connection_guard);
        next_valid = valid_connections;
        valid_connections = this;
    }
}

MirConnection::~MirConnection() noexcept
{
    if (channel)  // some tests don't have one
    {
        channel->discard_future_calls();
        channel->wait_for_outstanding_calls();
    }

    std::lock_guard<decltype(mutex)> lock(mutex);
    surface_map.reset();

    if (connect_result && connect_result->has_platform())
    {
        auto const& platform = connect_result->platform();
        for (int i = 0, end = platform.fd_size(); i != end; ++i)
            close(platform.fd(i));
    }
}

MirWaitHandle* MirConnection::create_surface(
    MirWindowSpec const& spec,
    MirWindowCallback callback,
    void * context)
{
    auto response = std::make_shared<mp::Surface>();
    auto c = std::make_shared<MirConnection::SurfaceCreationRequest>(callback, context, spec);
    c->wh->expect_result();
    auto const message = serialize_spec(spec);
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        surface_requests.emplace_back(c);
    }

    try
    {
        server.create_surface(&message, c->response.get(),
            gp::NewCallback(this, &MirConnection::surface_created, c.get()));
    }
    catch (std::exception const& ex)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        auto request = std::find_if(surface_requests.begin(), surface_requests.end(),
            [&](std::shared_ptr<MirConnection::SurfaceCreationRequest> c2) { return c.get() == c2.get(); });
        if (request != surface_requests.end())
        {
            auto id = next_error_id(lock);
            auto surf = std::make_shared<MirWindow>(
                std::string{"Error creating surface: "} + boost::diagnostic_information(ex), this, id, (*request)->wh);
            surface_map->insert(id, surf);
            auto wh = (*request)->wh;
            surface_requests.erase(request);

            lock.unlock();
            callback(surf.get(), context);
            wh->result_received();
        }
    }
    return c->wh.get();
}

mf::SurfaceId MirConnection::next_error_id(std::unique_lock<std::mutex> const&)
{
    return mf::SurfaceId{surface_error_id--};
}

void MirConnection::surface_created(SurfaceCreationRequest* request)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    //make sure this request actually was made.
    auto request_it = std::find_if(surface_requests.begin(), surface_requests.end(),
        [&request](std::shared_ptr<MirConnection::SurfaceCreationRequest> r){ return request == r.get(); });
    if (request_it == surface_requests.end())
        return;

    auto surface_proto = request->response;
    auto callback = request->cb;
    auto context = request->context;
    auto const& spec = request->spec;
    std::string name{spec.surface_name.is_set() ?
                     spec.surface_name.value() : ""};

    std::shared_ptr<MirBufferStream> default_stream {nullptr};
    if (surface_proto->buffer_stream().has_id())
    {
        try
        {
            default_stream = std::make_shared<mcl::BufferStream>(
                this, nullptr, request->wh, server, platform, surface_map, buffer_factory,
                surface_proto->buffer_stream(), make_perf_report(logger), name,
                mir::geometry::Size{surface_proto->width(), surface_proto->height()}, nbuffers);
            surface_map->insert(default_stream->rpc_id(), default_stream);
        }
        catch (std::exception const& error)
        {
            if (!surface_proto->has_error())
                surface_proto->set_error(error.what());
            // Clean up surface_proto's direct fds; BufferStream has cleaned up any owned by
            // surface_proto->buffer_stream()
            for (auto i = 0; i < surface_proto->fd_size(); i++)
                ::close(surface_proto->fd(i));
        }
    }

    std::shared_ptr<MirWindow> surf {nullptr};
    if (surface_proto->has_error() || !surface_proto->has_id())
    {
        std::string reason;
        if (surface_proto->has_error())
            reason += surface_proto->error();
        if (surface_proto->has_error() && !surface_proto->has_id())
            reason += " and ";
        if (!surface_proto->has_id())
            reason +=  "Server assigned surface no id";
        auto id = next_error_id(lock);
        surf = std::make_shared<MirWindow>(reason, this, id, request->wh);
        surface_map->insert(id, surf);
    }
    else
    {
        surf = std::make_shared<MirWindow>(
            this, server, &debug, default_stream, spec, *surface_proto, request->wh);
        surface_map->insert(mf::SurfaceId{surface_proto->id().value()}, surf);
    }

    callback(surf.get(), context);

    if (MirWindow::is_valid(surf.get()) && spec.width.is_set() && spec.height.is_set() && spec.event_handler.is_set())
    {
        mir::geometry::Size requested_size{spec.width.value(), spec.height.value()};
        mir::geometry::Size actual_size{(*surface_proto).width(), (*surface_proto).height()};
        auto event_handler = spec.event_handler.value();
        if (requested_size != actual_size)
        {
            auto event = mir::events::make_event(mir::frontend::SurfaceId{(*surface_proto).id().value()}, actual_size);
            lock.unlock();
            event_handler.callback(surf.get(), event.get(), event_handler.context);
            lock.lock();
        }
    }

    request->wh->result_received();
    surface_requests.erase(request_it);
}

char const * MirConnection::get_error_message()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (error_message.empty() && connect_result)
    {
        return connect_result->error().c_str();
    }

    return error_message.c_str();
}

void MirConnection::set_error_message(std::string const& error)
{
    error_message = error;
}


/* these structs exists to work around google protobuf being able to bind
 "only 0, 1, or 2 arguments in the NewCallback function */
struct MirConnection::SurfaceRelease
{
    MirWindow* surface;
    MirWaitHandle* handle;
    MirWindowCallback callback;
    void* context;
};

struct MirConnection::StreamRelease
{
    MirBufferStream* stream;
    MirWaitHandle* handle;
    MirBufferStreamCallback callback;
    void* context;
    int rpc_id;
    void* rs;
};

void MirConnection::released(StreamRelease data)
{
    if (data.callback)
        data.callback(data.stream, data.context);
    if (data.handle)
        data.handle->result_received();

    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        if (data.rs)
            surface_map->erase(data.rs);
        surface_map->erase(mf::BufferStreamId(data.rpc_id));
    }
}

void MirConnection::released(SurfaceRelease data)
{
    surface_map->erase(mf::BufferStreamId(data.surface->id()));
    surface_map->erase(mf::SurfaceId(data.surface->id()));
    data.callback(data.surface, data.context);
    data.handle->result_received();
}

MirWaitHandle* MirConnection::release_surface(
        MirWindow *surface,
        MirWindowCallback callback,
        void * context)
{
    auto wait_handle = std::make_unique<MirWaitHandle>();
    auto raw_wait_handle = wait_handle.get();
    {
        std::lock_guard<decltype(release_wait_handle_guard)> rel_lock(release_wait_handle_guard);
        release_wait_handles.push_back(std::move(wait_handle));
    }

    if (!mir_window_is_valid(surface))
    {
        raw_wait_handle->expect_result();
        raw_wait_handle->result_received();
        callback(surface, context);
        auto id = surface->id();
        surface_map->erase(mf::SurfaceId(id));
        return raw_wait_handle;
    }

    SurfaceRelease surf_release{surface, raw_wait_handle, callback, context};

    mp::SurfaceId message;
    message.set_value(surface->id());

    raw_wait_handle->expect_result();
    server.release_surface(&message, void_response.get(),
                           gp::NewCallback(this, &MirConnection::released, surf_release));


    return raw_wait_handle;
}

MirPromptSession* MirConnection::create_prompt_session()
{
    return new MirPromptSession(display_server(), event_handler_register);
}

void MirConnection::connected(MirConnectedCallback callback, void * context)
{
    try
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (!connect_result->has_platform() || !connect_result->has_display_configuration())
            set_error_message("Failed to connect: not accepted by server");

        connect_done = true;

        translation_ext = MirExtensionWindowCoordinateTranslationV1{ translate_coordinates };
        graphics_module_extension = MirExtensionGraphicsModuleV1 { get_graphics_module };

        for ( auto i = 0; i < connect_result->extension().size(); i++)
        {
            auto& ex = connect_result->extension(i);
            std::vector<int> versions;
            for ( auto j = 0; j < connect_result->extension(i).version().size(); j++ )
                versions.push_back(connect_result->extension(i).version(j));
            extensions.push_back({ex.name(), versions});
        }

        /*
         * We need to create the client platform after the connection has been
         * established, to ensure that the client platform has access to all
         * needed data (e.g. platform package).
         */
        auto default_lifecycle_event_handler = [this](MirLifecycleState transition)
            {
                if (transition == mir_lifecycle_connection_lost && !disconnecting)
                {
                    /*
                     * We need to use kill() instead of raise() to ensure the signal
                     * is dispatched to the process even if it's blocked in the current
                     * thread.
                     */
                    kill(getpid(), SIGHUP);
                }
            };

        platform = client_platform_factory->create_client_platform(this);
        native_display = platform->create_egl_native_display();
        display_configuration->set_configuration(connect_result->display_configuration());
        lifecycle_control->set_callback(default_lifecycle_event_handler);
        ping_handler->set_callback([this](int32_t serial)
        {
            this->pong(serial);
        });

        if (connect_result->has_input_configuration())
        {
            input_devices->update_devices(connect_result->input_configuration());
        }
    }
    catch (std::exception const& e)
    {
        connect_result->set_error(std::string{"Failed to process connect response: "} +
                                 boost::diagnostic_information(e));
    }

    callback(this, context);
    connect_wait_handle.result_received();
}

MirWaitHandle* MirConnection::connect(
    const char* app_name,
    MirConnectedCallback callback,
    void * context)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        connect_parameters->set_application_name(app_name);
        connect_wait_handle.expect_result();
    }

    server.connect(
        connect_parameters.get(),
        connect_result.get(),
        google::protobuf::NewCallback(
            this, &MirConnection::connected, callback, context));
    return &connect_wait_handle;
}

void MirConnection::done_disconnect()
{
    /* todo: keeping all MirWaitHandles from a release surface until the end of the connection
       is a kludge until we have a better story about the lifetime of MirWaitHandles */
    {
        std::lock_guard<decltype(release_wait_handle_guard)> lock(release_wait_handle_guard);
        release_wait_handles.clear();
    }

    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        surface_map.reset();
    }

    // Ensure no racy lifecycle notifications can happen after disconnect completes
    lifecycle_control->set_callback([](MirLifecycleState){});
    disconnect_wait_handle.result_received();
}

MirWaitHandle* MirConnection::disconnect()
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        disconnecting = true;
    }
    surface_map->with_all_streams_do([](MirBufferStream* receiver)
    {
        receiver->buffer_unavailable();
    });

    disconnect_wait_handle.expect_result();
    server.disconnect(ignored.get(), ignored.get(),
                      google::protobuf::NewCallback(this, &MirConnection::done_disconnect));

    if (channel)
        channel->discard_future_calls();

    return &disconnect_wait_handle;
}

void MirConnection::done_platform_operation(
    MirPlatformOperationCallback callback, void* context)
{
    auto reply = new MirPlatformMessage(platform_operation_reply->opcode());

    set_error_message(platform_operation_reply->error());

    auto const char_data = static_cast<char const*>(platform_operation_reply->data().data());
    reply->data.assign(char_data, char_data + platform_operation_reply->data().size());
    reply->fds.assign(
        platform_operation_reply->fd().data(),
        platform_operation_reply->fd().data() + platform_operation_reply->fd().size());

    callback(this, reply, context);

    platform_operation_wait_handle.result_received();
}

MirWaitHandle* MirConnection::platform_operation(
    MirPlatformMessage const* request,
    MirPlatformOperationCallback callback, void* context)
{
    auto const client_response = platform->platform_operation(request);
    if (client_response)
    {
        set_error_message("");
        callback(this, client_response, context);
        return nullptr;
    }

    mp::PlatformOperationMessage protobuf_request;

    protobuf_request.set_opcode(request->opcode);
    protobuf_request.set_data(request->data.data(), request->data.size());
    for (size_t i = 0; i != request->fds.size(); ++i)
        protobuf_request.add_fd(request->fds[i]);

    platform_operation_wait_handle.expect_result();
    server.platform_operation(
        &protobuf_request,
        platform_operation_reply.get(),
        google::protobuf::NewCallback(this, &MirConnection::done_platform_operation,
                                      callback, context));

    return &platform_operation_wait_handle;
}


bool MirConnection::is_valid(MirConnection *connection)
{
    {
        std::unique_lock<std::mutex> lock(connection_guard);
        for (auto current = valid_connections; current; current = current->next_valid)
        {
            if (connection == current)
            {
                lock.unlock();
                std::lock_guard<decltype(connection->mutex)> lock(connection->mutex);
                return !connection->connect_result->has_error();
            }
        }
    }
    return false;
}

void MirConnection::populate(MirPlatformPackage& platform_package)
{
    platform->populate(platform_package);
}

void MirConnection::populate_server_package(MirPlatformPackage& platform_package)
{
    // connect_result is write-once: once it's valid, we don't need to lock
    // to use it.
    if (connect_done && !connect_result->has_error() && connect_result->has_platform())
    {
        auto const& platform = connect_result->platform();

        platform_package.data_items = platform.data_size();
        for (int i = 0; i != platform.data_size(); ++i)
            platform_package.data[i] = platform.data(i);

        platform_package.fd_items = platform.fd_size();
        for (int i = 0; i != platform.fd_size(); ++i)
            platform_package.fd[i] = platform.fd(i);
    }
    else
    {
        platform_package.data_items = 0;
        platform_package.fd_items = 0;
    }
}

void MirConnection::populate_graphics_module(MirModuleProperties& properties)
{
    // connect_result is write-once: once it's valid, we don't need to lock
    // to use it.
    if (connect_done &&
        !connect_result->has_error() &&
        connect_result->has_platform() &&
        connect_result->platform().has_graphics_module())
    {
        auto const& graphics_module = connect_result->platform().graphics_module();
        properties.name = graphics_module.name().c_str();
        properties.major_version = graphics_module.major_version();
        properties.minor_version = graphics_module.minor_version();
        properties.micro_version = graphics_module.micro_version();
        properties.filename = graphics_module.file().c_str();
    }
    else
    {
        properties.name = "(unknown)";
        properties.major_version = 0;
        properties.minor_version = 0;
        properties.micro_version = 0;
        properties.filename = nullptr;
    }
}

MirDisplayConfiguration* MirConnection::create_copy_of_display_config()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return display_configuration->copy_to_client();
}

void MirConnection::available_surface_formats(
    MirPixelFormat* formats,
    unsigned int formats_size,
    unsigned int& valid_formats)
{
    valid_formats = 0;

    std::lock_guard<decltype(mutex)> lock(mutex);
    if (!connect_result->has_error())
    {
        valid_formats = std::min(
            static_cast<unsigned int>(connect_result->surface_pixel_format_size()),
            formats_size);

        for (auto i = 0u; i < valid_formats; i++)
        {
            formats[i] = static_cast<MirPixelFormat>(connect_result->surface_pixel_format(i));
        }
    }
}

void MirConnection::stream_created(StreamCreationRequest* request_raw)
{
    std::shared_ptr<StreamCreationRequest> request {nullptr};
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto stream_it = std::find_if(stream_requests.begin(), stream_requests.end(),
            [&request_raw] (std::shared_ptr<StreamCreationRequest> const& req)
            { return req.get() == request_raw; });
        if (stream_it == stream_requests.end())
            return;
        request = *stream_it;
        stream_requests.erase(stream_it);
    }
    auto& protobuf_bs = request->response;

    if (!protobuf_bs->has_id())
    {
        if (!protobuf_bs->has_error())
            protobuf_bs->set_error("no ID in response (disconnected?)");
    }

    if (protobuf_bs->has_error())
    {
        for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
            ::close(protobuf_bs->buffer().fd(i));
        stream_error(
            std::string{"Error processing buffer stream response: "} + protobuf_bs->error(),
            request);
        return;
    }

    try
    {
        auto stream = std::make_shared<mcl::BufferStream>(
            this, request->rs, request->wh, server, platform, surface_map, buffer_factory,
            *protobuf_bs, make_perf_report(logger), std::string{},
            mir::geometry::Size{request->parameters.width(), request->parameters.height()}, nbuffers);
        surface_map->insert(mf::BufferStreamId(protobuf_bs->id().value()), stream);

        if (request->mbs_callback)
            request->mbs_callback(stream.get(), request->context);

        request->wh->result_received();
    }
    catch (std::exception const& error)
    {
        stream_error(
            std::string{"Error processing buffer stream creating response:"} + boost::diagnostic_information(error),
            request);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirWaitHandle* MirConnection::create_client_buffer_stream(
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    MirRenderSurface* render_surface,
    MirBufferStreamCallback mbs_callback,
    void *context)
{
    mp::BufferStreamParameters params;
    params.set_width(width);
    params.set_height(height);
    params.set_pixel_format(format);
    params.set_buffer_usage(buffer_usage);

    auto request = std::make_shared<StreamCreationRequest>(render_surface,
                                                           mbs_callback,
                                                           context,
                                                           params);
    request->wh->expect_result();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        stream_requests.push_back(request);
    }

    try
    {
        server.create_buffer_stream(&params, request->response.get(),
            gp::NewCallback(this, &MirConnection::stream_created, request.get()));
    } catch (std::exception& e)
    {
        //if this throws, our socket code will run the closure, which will make an error object.
        //its nicer to return an stream with a error message, so just ignore the exception.
    }

    return request->wh.get();
}

std::shared_ptr<mir::client::BufferStream> MirConnection::create_client_buffer_stream_with_id(
    int width, int height,
    MirRenderSurface* render_surface,
    mir::protobuf::BufferStream const& a_protobuf_bs)
{
    auto stream = std::make_shared<mcl::BufferStream>(
        this, render_surface, nullptr, server, platform, surface_map, buffer_factory,
        a_protobuf_bs, make_perf_report(logger), std::string{},
        mir::geometry::Size{width, height}, nbuffers);
    surface_map->insert(render_surface->stream_id(), stream);
    return stream;
}
#pragma GCC diagnostic pop

void MirConnection::render_surface_error(std::string const& error_msg, std::shared_ptr<RenderSurfaceCreationRequest> const& request)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    mf::BufferStreamId id(next_error_id(lock).as_value());

    auto rs = std::make_shared<mcl::ErrorRenderSurface>(error_msg, this);
    surface_map->insert(request->native_window.get(), rs);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if (request->callback)
        request->callback(
            static_cast<MirRenderSurface*>(request->native_window.get()),
            request->context);
#pragma GCC diagnostic pop

    request->wh->result_received();
}

void MirConnection::stream_error(std::string const& error_msg, std::shared_ptr<StreamCreationRequest> const& request)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    mf::BufferStreamId id(next_error_id(lock).as_value());

    auto stream = std::make_shared<mcl::ErrorStream>(error_msg, this, id, request->wh);
    surface_map->insert(id, stream);

    if (request->mbs_callback)
    {
        request->mbs_callback(stream.get(), request->context);
    }

    request->wh->result_received();
}

std::shared_ptr<MirBufferStream> MirConnection::make_consumer_stream(
   mp::BufferStream const& protobuf_bs)
{
    return std::make_shared<mcl::ScreencastStream>(
        this, server, platform, protobuf_bs);
}

EGLNativeDisplayType MirConnection::egl_native_display()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return *native_display;
}

MirPixelFormat MirConnection::egl_pixel_format(EGLDisplay disp, EGLConfig conf) const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return platform->get_egl_pixel_format(disp, conf);
}

void MirConnection::register_lifecycle_event_callback(MirLifecycleEventCallback callback, void* context)
{
    lifecycle_control->set_callback(std::bind(callback, this, std::placeholders::_1, context));
}

void MirConnection::register_ping_event_callback(MirPingEventCallback callback, void* context)
{
    ping_handler->set_callback(std::bind(callback, this, std::placeholders::_1, context));
}

void MirConnection::register_error_callback(MirErrorCallback callback, void* context)
{
    error_handler->set_callback(std::bind(callback, this, std::placeholders::_1, context));
}

void MirConnection::pong(int32_t serial)
{
    mp::PingEvent pong;
    pong.set_serial(serial);
    server.pong(&pong, void_response.get(), pong_callback.get());
}

void MirConnection::register_display_change_callback(MirDisplayConfigCallback callback, void* context)
{
    display_configuration->set_display_change_handler(std::bind(callback, this, context));
}

bool MirConnection::validate_user_display_config(MirDisplayConfiguration const* config)
{
    MirDisplayConfigurationStore orig_config{display_configuration->copy_to_client()};

    if ((!config) || (config->num_outputs == 0) || (config->outputs == NULL) ||
        (config->num_outputs > orig_config->num_outputs))
    {
        return false;
    }

    for(auto i = 0u; i < config->num_outputs; i++)
    {
        auto const& output = config->outputs[i];
        auto const& orig_output = orig_config->outputs[i];

        if (output.output_id != orig_output.output_id)
            return false;

        if (output.connected && output.current_mode >= orig_output.num_modes)
            return false;
    }

    return true;
}

void MirConnection::done_display_configure()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    set_error_message(display_configuration_response->error());

    if (!display_configuration_response->has_error())
        display_configuration->set_configuration(*display_configuration_response);

    return configure_display_wait_handle.result_received();
}

MirWaitHandle* MirConnection::configure_display(MirDisplayConfiguration* config)
{
    if (!validate_user_display_config(config))
    {
        return NULL;
    }

    mp::DisplayConfiguration request;
    populate_protobuf_display_configuration(request, config);

    configure_display_wait_handle.expect_result();
    server.configure_display(&request, display_configuration_response.get(),
        google::protobuf::NewCallback(this, &MirConnection::done_display_configure));

    return &configure_display_wait_handle;
}

MirWaitHandle* MirConnection::set_base_display_configuration(MirDisplayConfiguration const* config)
{
    if (!validate_user_display_config(config))
    {
        return NULL;
    }

    mp::DisplayConfiguration request;
    populate_protobuf_display_configuration(request, config);

    set_base_display_configuration_wait_handle.expect_result();
    server.set_base_display_configuration(&request, set_base_display_configuration_response.get(),
        google::protobuf::NewCallback(this, &MirConnection::done_set_base_display_configuration));

    return &set_base_display_configuration_wait_handle;
}

namespace
{
template <class T>
struct HandleError
{
    std::unique_ptr<T> result;
    std::function<void(T const&)> on_error;
};

template <class T>
void handle_structured_error(HandleError<T>* handler)
{
    if (handler->result->has_structured_error())
    {
        handler->on_error(*handler->result);
    }
    delete handler;
}

template <class T>
HandleError<T>* create_stored_error_result(std::shared_ptr<mcl::ErrorHandler> const& error_handler)
{
    auto store_error_result = new HandleError<T>;
    store_error_result->result = std::make_unique<T>();
    store_error_result->on_error = [error_handler](T const& message)
    {
        MirError const error{
            static_cast<MirErrorDomain>(message.structured_error().domain()),
            message.structured_error().code()};
        (*error_handler)(&error);
    };

    return store_error_result;
}
}

void MirConnection::preview_base_display_configuration(
    mp::DisplayConfiguration const& configuration,
    std::chrono::seconds timeout)
{
    mp::PreviewConfiguration request;

    request.mutable_configuration()->CopyFrom(configuration);
    request.set_timeout(timeout.count());

    auto store_error_result = create_stored_error_result<mp::Void>(error_handler);

    server.preview_base_display_configuration(
        &request,
        store_error_result->result.get(),
        google::protobuf::NewCallback(&handle_structured_error, store_error_result));
}

void MirConnection::cancel_base_display_configuration_preview()
{
    mp::Void request;

    auto store_error_result = create_stored_error_result<mp::Void>(error_handler);

    server.cancel_base_display_configuration_preview(
        &request,
        store_error_result->result.get(),
        google::protobuf::NewCallback(&handle_structured_error, store_error_result));
}

void MirConnection::configure_session_display(mp::DisplayConfiguration const& configuration)
{
    auto store_error_result = create_stored_error_result<mp::DisplayConfiguration>(error_handler);

    server.configure_display(
        &configuration,
        store_error_result->result.get(),
        google::protobuf::NewCallback(&handle_structured_error, store_error_result));
}

void MirConnection::remove_session_display()
{
    mp::Void request;
    auto store_error_result = create_stored_error_result<mp::Void>(error_handler);

    server.remove_session_configuration(
        &request,
        store_error_result->result.get(),
        google::protobuf::NewCallback(&handle_structured_error, store_error_result));
}

void MirConnection::confirm_base_display_configuration(
    mp::DisplayConfiguration const& configuration)
{
    server.confirm_base_display_configuration(
        &configuration,
        set_base_display_configuration_response.get(),
        google::protobuf::NewCallback(google::protobuf::DoNothing));
}

void MirConnection::done_set_base_display_configuration()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    set_error_message(set_base_display_configuration_response->error());

    return set_base_display_configuration_wait_handle.result_received();
}

mir::client::rpc::DisplayServer& MirConnection::display_server()
{
    return server;
}

MirWaitHandle* MirConnection::release_buffer_stream(
    MirBufferStream* stream,
    MirBufferStreamCallback callback,
    void *context)
{
    auto wait_handle = std::make_unique<MirWaitHandle>();
    auto raw_wait_handle = wait_handle.get();

    StreamRelease stream_release{stream, raw_wait_handle, callback, context, stream->rpc_id().as_value(), nullptr };

    mp::BufferStreamId buffer_stream_id;
    buffer_stream_id.set_value(stream->rpc_id().as_value());

    {
        std::lock_guard<decltype(release_wait_handle_guard)> rel_lock(release_wait_handle_guard);
        release_wait_handles.push_back(std::move(wait_handle));
    }

    raw_wait_handle->expect_result();
    server.release_buffer_stream(
        &buffer_stream_id, void_response.get(),
        google::protobuf::NewCallback(this, &MirConnection::released, stream_release));
    return raw_wait_handle;
}

void MirConnection::release_consumer_stream(MirBufferStream* stream)
{
    surface_map->erase(stream->rpc_id());
}

std::unique_ptr<mir::protobuf::DisplayConfiguration> MirConnection::snapshot_display_configuration() const
{
    return display_configuration->take_snapshot();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
std::shared_ptr<mcl::PresentationChain> MirConnection::create_presentation_chain_with_id(
    MirRenderSurface* render_surface,
    mir::protobuf::BufferStream const& a_protobuf_bs)
{
    if (!client_buffer_factory)
        client_buffer_factory = platform->create_buffer_factory();
    auto chain = std::make_shared<mcl::PresentationChain>(
        this, a_protobuf_bs.id().value(), server, client_buffer_factory, buffer_factory);

    surface_map->insert(render_surface->stream_id(), chain);
    return chain;
}
#pragma GCC diagnostic pop

void MirConnection::allocate_buffer(
    geom::Size size, MirPixelFormat format,
    MirBufferCallback callback, void* context)
{
    mp::BufferAllocation request;
    auto buffer_request = request.add_buffer_requests();
    buffer_request->set_width(size.width.as_int());
    buffer_request->set_height(size.height.as_int());
    buffer_request->set_pixel_format(format);
    buffer_request->set_buffer_usage(mir_buffer_usage_software);

    if (!client_buffer_factory)
        client_buffer_factory = platform->create_buffer_factory();
    buffer_factory->expect_buffer(
        client_buffer_factory, this,
        size, format, mir_buffer_usage_software,
        callback, context);
    server.allocate_buffers(&request, ignored.get(), gp::NewCallback(ignore));
}

void MirConnection::allocate_buffer(
    geom::Size size, uint32_t native_format, uint32_t native_flags,
    MirBufferCallback callback, void* context)
{
    mp::BufferAllocation request;
    auto buffer_request = request.add_buffer_requests();
    buffer_request->set_width(size.width.as_int());
    buffer_request->set_height(size.height.as_int());
    buffer_request->set_native_format(native_format);
    buffer_request->set_flags(native_flags);

    if (!client_buffer_factory)
        client_buffer_factory = platform->create_buffer_factory();
    buffer_factory->expect_buffer(
        client_buffer_factory, this,
        size, native_format, native_flags,
        callback, context);
    server.allocate_buffers(&request, ignored.get(), gp::NewCallback(ignore));
}

void MirConnection::release_buffer(mcl::MirBuffer* buffer)
{
    if (buffer->valid())
    {
        mp::BufferRelease request;
        auto released_buffer = request.add_buffers();
        released_buffer->set_buffer_id(buffer->rpc_id());
        server.release_buffers(&request, ignored.get(), gp::NewCallback(ignore));
    }

    surface_map->erase(buffer->rpc_id());
}

void MirConnection::release_render_surface_with_content(
    void* render_surface)
{
    auto rs = surface_map->render_surface(render_surface);
    if (!rs->valid())
    {
        surface_map->erase(render_surface);
        return;
    }

    StreamRelease stream_release{
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        rs->stream_id().as_value(),
        render_surface};

    mp::BufferStreamId buffer_stream_id;
    buffer_stream_id.set_value(rs->stream_id().as_value());

    server.release_buffer_stream(
        &buffer_stream_id, void_response.get(),
        google::protobuf::NewCallback(this, &MirConnection::released, stream_release));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void MirConnection::render_surface_created(RenderSurfaceCreationRequest* request_raw)
{
    std::string static const error_msg = "Error creating MirRenderSurface: ";
    std::shared_ptr<RenderSurfaceCreationRequest> request {nullptr};
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto rs_it = std::find_if(render_surface_requests.begin(), render_surface_requests.end(),
            [&request_raw] (std::shared_ptr<RenderSurfaceCreationRequest> const& req)
            { return req.get() == request_raw; });
        if (rs_it == render_surface_requests.end())
            return;
        request = *rs_it;
        render_surface_requests.erase(rs_it);
    }
    auto& protobuf_bs = request->response;

    if (!protobuf_bs->has_id())
    {
        if (!protobuf_bs->has_error())
            protobuf_bs->set_error("no ID in response (disconnected?)");
    }

    if (protobuf_bs->has_error())
    {
        for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
            ::close(protobuf_bs->buffer().fd(i));
        render_surface_error(error_msg + protobuf_bs->error(), request);
        return;
    }

    try
    {
        auto rs = std::make_shared<mcl::RenderSurface>(
            this,
            request->native_window,
            platform,
            protobuf_bs,
            request->logical_size);
        surface_map->insert(request->native_window.get(), rs);

        if (request->callback)
        {
            request->callback(
                static_cast<MirRenderSurface*>(request->native_window.get()),
                    request->context);
        }

        request->wh->result_received();
    }
    catch (std::exception const& error)
    {
        render_surface_error(error_msg + error.what(), request);
    }
}

auto MirConnection::create_render_surface_with_content(
    mir::geometry::Size logical_size,
    MirRenderSurfaceCallback callback,
    void* context)
-> MirRenderSurface*
{
    mir::protobuf::BufferStreamParameters params;
    params.set_width(logical_size.width.as_int());
    params.set_height(logical_size.height.as_int());
    params.set_pixel_format(connect_result->surface_pixel_format(0));
    params.set_buffer_usage(-1);

    auto nw = platform->create_egl_native_window(nullptr);
    auto request = std::make_shared<RenderSurfaceCreationRequest>(
        callback, context, nw, logical_size);

    request->wh->expect_result();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        render_surface_requests.push_back(request);
    }

    try
    {
        server.create_buffer_stream(&params, request->response.get(),
            gp::NewCallback(this, &MirConnection::render_surface_created, request.get()));
    } catch (std::exception& e)
    {
        //if this throws, our socket code will run the closure, which will make an error object.
    }

    return static_cast<MirRenderSurface*>(nw.get());
}
#pragma GCC diagnostic pop

void* MirConnection::request_interface(char const* name, int version)
{
    if (!platform)
        BOOST_THROW_EXCEPTION(std::invalid_argument("cannot query extensions before connecting to server"));

    auto supported = std::find_if(extensions.begin(), extensions.end(),
        [&](auto& e) {
            return e.name == std::string{name} &&
                std::find(e.version.begin(), e.version.end(), version) != e.version.end();
        });
    if (supported == extensions.end())
        return nullptr;

    if (!strcmp(name, "mir_extension_window_coordinate_translation") && (version == 1) && translation_ext.is_set())
        return &translation_ext.value();
    if (!strcmp(name, "mir_drag_and_drop") && (version == 1))
        return const_cast<MirDragAndDropV1*>(mir::drag_and_drop::v1);

    //this extension should move to the platform plugin.
    if (!strcmp(name, "mir_extension_graphics_module") && (version == 1))
        return &graphics_module_extension.value();
    return platform->request_interface(name, version);
}

void MirConnection::apply_input_configuration(MirInputConfig const* config)
{
    auto store_error_result = create_stored_error_result<mp::Void>(error_handler);

    mp::InputConfigurationRequest request;
    request.set_input_configuration(mi::serialize_input_config(*config));

    server.apply_input_configuration(&request,
                                     store_error_result->result.get(),
                                     gp::NewCallback(&handle_structured_error, store_error_result));
}

void MirConnection::set_base_input_configuration(MirInputConfig const* config)
{
    auto store_error_result = create_stored_error_result<mp::Void>(error_handler);

    mp::InputConfigurationRequest request;
    request.set_input_configuration(mi::serialize_input_config(*config));

    server.set_base_input_configuration(&request,
                                        store_error_result->result.get(),
                                        gp::NewCallback(&handle_structured_error, store_error_result));
}

void MirConnection::enumerate_extensions(
    void* context,
    void (*enumerator)(void* context, char const* extension, int version))
{
    for(auto const& extension : extensions)
    {
        for(auto const version : extension.version)
        {
            enumerator(context, extension.name.c_str(), version);
        }
    }
}
