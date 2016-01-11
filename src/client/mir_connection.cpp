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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_connection.h"
#include "mir_surface.h"
#include "mir_prompt_session.h"
#include "mir_protobuf.pb.h"
#include "make_protobuf_object.h"
#include "mir_toolkit/mir_platform_message.h"
#include "mir/client_platform.h"
#include "mir/client_platform_factory.h"
#include "rpc/mir_basic_rpc_channel.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "connection_configuration.h"
#include "display_configuration.h"
#include "connection_surface_map.h"
#include "lifecycle_control.h"
#include "error_stream.h"
#include "buffer_stream.h"
#include "perf_report.h"
#include "logging/perf_report.h"
#include "lttng/perf_report.h"

#include "mir/events/event_builders.h"
#include "mir/logging/logger.h"

#include <algorithm>
#include <cstddef>
#include <unistd.h>
#include <signal.h>

#include <boost/exception/diagnostic_information.hpp>

namespace mcl = mir::client;
namespace md = mir::dispatch;
namespace mircv = mir::input::receiver;
namespace mev = mir::events;
namespace gp = google::protobuf;
namespace mf = mir::frontend;
namespace mp = mir::protobuf;
namespace ml = mir::logging;

namespace
{
std::shared_ptr<mcl::PerfReport>
make_perf_report(std::shared_ptr<ml::Logger> const& logger)
{
    // TODO: It seems strange that this directly uses getenv
    const char* report_target = getenv("MIR_CLIENT_PERF_REPORT");
    if (report_target && !strncmp(report_target, "log", strlen(report_target)))
    {
        return std::make_shared<mcl::logging::PerfReport>(logger);
    }
    else if (report_target && !strncmp(report_target, "lttng", strlen(report_target)))
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
    if (nbuffers_opt && !strncmp(nbuffers_opt, "2", strlen(nbuffers_opt)))
        return 2u;
    return 3u;
}

struct OnScopeExit
{
    ~OnScopeExit() { f(); }
    std::function<void()> const f;
};
mir::protobuf::SurfaceParameters serialize_spec(MirSurfaceSpec const& spec)
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
    SERIALIZE_OPTION_IF_SET(min_width);
    SERIALIZE_OPTION_IF_SET(min_height);
    SERIALIZE_OPTION_IF_SET(max_width);
    SERIALIZE_OPTION_IF_SET(max_height);
    SERIALIZE_OPTION_IF_SET(width_inc);
    SERIALIZE_OPTION_IF_SET(height_inc);
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
        platform(client_platform_factory->create_client_platform(this)),
        input_platform(conf.the_input_platform()),
        display_configuration(conf.the_display_configuration()),
        lifecycle_control(conf.the_lifecycle_control()),
        ping_handler{conf.the_ping_handler()},
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
    // We don't die while if are pending callbacks (as they touch this).
    // But, if after 500ms we don't get a call, assume it won't happen.
    connect_wait_handle.wait_for_pending(std::chrono::milliseconds(500));

    surface_map.reset();

    std::lock_guard<decltype(mutex)> lock(mutex);
    if (connect_result && connect_result->has_platform())
    {
        auto const& platform = connect_result->platform();
        for (int i = 0, end = platform.fd_size(); i != end; ++i)
            close(platform.fd(i));
    }
}

MirWaitHandle* MirConnection::create_surface(
    MirSurfaceSpec const& spec,
    mir_surface_callback callback,
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
            auto surf = std::make_shared<MirSurface>(
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
    std::shared_ptr<MirSurface> surf {nullptr};
    std::shared_ptr<mcl::ClientBufferStream> stream {nullptr};
    //make sure this request actually was made.
    auto request_it = std::find_if(surface_requests.begin(), surface_requests.end(),
        [&request](std::shared_ptr<MirConnection::SurfaceCreationRequest> r){ return request == r.get(); });
    if (request_it == surface_requests.end())
        return;

    auto surface_proto = request->response; 
    auto callback = request->cb;
    auto context = request->context;
    auto const& spec = request->spec;

    try
    {
        stream = std::make_shared<mcl::BufferStream>(
            this, request->wh, server, mcl::BufferStreamMode::Producer, platform,
            surface_proto->buffer_stream(), make_perf_report(logger), std::string{},
            mir::geometry::Size{surface_proto->width(), surface_proto->height()}, nbuffers);
    }
    catch (std::exception const& error)
    {
        if (!surface_proto->has_error())
            surface_proto->set_error(error.what());
        // failed to create buffer_stream, so clean up FDs it doesn't own
        for (auto i = 0; i < surface_proto->fd_size(); i++)
            ::close(surface_proto->fd(i));
        if (surface_proto->has_buffer_stream() && surface_proto->buffer_stream().has_buffer())
            for (int i = 0; i < surface_proto->buffer_stream().buffer().fd_size(); i++)
                ::close(surface_proto->buffer_stream().buffer().fd(i));
    }

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
        surf = std::make_shared<MirSurface>(reason, this, id, request->wh);
        surface_map->insert(id, surf);
    }
    else
    {
        surf = std::make_shared<MirSurface>(
            this, server, &debug, stream, input_platform, spec, *surface_proto, request->wh);

        surface_map->insert(mf::SurfaceId{surface_proto->id().value()}, surf);
        surface_map->insert(mf::BufferStreamId{surface_proto->id().value()}, stream);
    }

    callback(surf.get(), context);
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
    MirSurface* surface;
    MirWaitHandle* handle;
    mir_surface_callback callback;
    void* context;
};

struct MirConnection::StreamRelease
{
    mcl::ClientBufferStream* stream;
    MirWaitHandle* handle;
    mir_buffer_stream_callback callback;
    void* context;
};

void MirConnection::released(StreamRelease data)
{
    if (data.callback)
        data.callback(reinterpret_cast<MirBufferStream*>(data.stream), data.context);
    data.handle->result_received();
    surface_map->erase(mf::BufferStreamId(data.stream->rpc_id()));
}

void MirConnection::released(SurfaceRelease data)
{
    // releasing this surface from surface_map means that it will no longer receive events
    // If it's still focused, send an unfocused event before we kill it entirely
    if (data.surface->attrib(mir_surface_attrib_focus) == mir_surface_focused)
    {
        auto unfocus = mev::make_event(mir::frontend::SurfaceId{data.surface->id()}, mir_surface_attrib_focus, mir_surface_unfocused);
        data.surface->handle_event(*unfocus);
    }
    data.callback(data.surface, data.context);
    data.handle->result_received();
    surface_map->erase(mf::BufferStreamId(data.surface->id()));
    surface_map->erase(mf::SurfaceId(data.surface->id()));
}

MirWaitHandle* MirConnection::release_surface(
        MirSurface *surface,
        mir_surface_callback callback,
        void * context)
{
    auto new_wait_handle = new MirWaitHandle;
    {
        std::lock_guard<decltype(release_wait_handle_guard)> rel_lock(release_wait_handle_guard);
        release_wait_handles.push_back(new_wait_handle);
    }

    if (strncmp(surface->get_error_message(), "", 1))
    {
        new_wait_handle->expect_result();
        new_wait_handle->result_received();
        callback(surface, context);
        auto id = surface->id();
        surface_map->erase(mf::SurfaceId(id));
        return new_wait_handle;    
    }

    SurfaceRelease surf_release{surface, new_wait_handle, callback, context};

    mp::SurfaceId message;
    message.set_value(surface->id());

    new_wait_handle->expect_result();
    server.release_surface(&message, void_response.get(),
                           gp::NewCallback(this, &MirConnection::released, surf_release));


    return new_wait_handle;
}

MirPromptSession* MirConnection::create_prompt_session()
{
    return new MirPromptSession(display_server(), event_handler_register);
}

void MirConnection::connected(mir_connected_callback callback, void * context)
{
    try
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (!connect_result->has_platform() || !connect_result->has_display_configuration())
            set_error_message("Failed to connect: not accepted by server");

        connect_done = true;

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
    mir_connected_callback callback,
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
        for (auto handle : release_wait_handles)
            delete handle;
    }
    surface_map.reset();

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
    disconnect_wait_handle.expect_result();
    server.disconnect(ignored.get(), ignored.get(),
                      google::protobuf::NewCallback(this, &MirConnection::done_disconnect));

    return &disconnect_wait_handle;
}

void MirConnection::done_platform_operation(
    mir_platform_operation_callback callback, void* context)
{
    auto reply = mir_platform_message_create(platform_operation_reply->opcode());

    set_error_message(platform_operation_reply->error());

    mir_platform_message_set_data(
        reply,
        platform_operation_reply->data().data(),
        platform_operation_reply->data().size());

    mir_platform_message_set_fds(
        reply,
        platform_operation_reply->fd().data(),
        platform_operation_reply->fd().size());

    callback(this, reply, context);

    platform_operation_wait_handle.result_received();
}

MirWaitHandle* MirConnection::platform_operation(
    MirPlatformMessage const* request,
    mir_platform_operation_callback callback, void* context)
{
    auto const client_response = platform->platform_operation(request);
    if (client_response)
    {
        set_error_message("");
        callback(this, client_response, context);
        return nullptr;
    }

    mp::PlatformOperationMessage protobuf_request;

    protobuf_request.set_opcode(mir_platform_message_get_opcode(request));
    auto const request_data = mir_platform_message_get_data(request);
    auto const request_fds = mir_platform_message_get_fds(request);

    protobuf_request.set_data(request_data.data, request_data.size);
    for (size_t i = 0; i != request_fds.num_fds; ++i)
        protobuf_request.add_fd(request_fds.fds[i]);

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
    auto stream_it = std::find_if(stream_requests.begin(), stream_requests.end(),
        [&request_raw] (std::shared_ptr<StreamCreationRequest> const& req)
        { return req.get() == request_raw; });
    if (stream_it == stream_requests.end())
        return;

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
<<<<<<< TREE
=======

>>>>>>> MERGE-SOURCE
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
            this, request->wh, server, mcl::BufferStreamMode::Producer, platform,
<<<<<<< TREE
            *protobuf_bs, make_perf_report(logger), std::string{},
            mir::geometry::Size{request->parameters.width(), request->parameters.height()}, nbuffers);
=======
            *protobuf_bs, make_perf_report(logger), std::string{}, mir::geometry::Size{0,0}, nbuffers);
>>>>>>> MERGE-SOURCE
        surface_map->insert(mf::BufferStreamId(protobuf_bs->id().value()), stream);

        if (request->callback)
            request->callback(reinterpret_cast<MirBufferStream*>(dynamic_cast<mcl::ClientBufferStream*>(stream.get())), request->context);
        request->wh->result_received();
    }
    catch (std::exception const& error)
    {
        for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
            ::close(protobuf_bs->buffer().fd(i));

        stream_error(
            std::string{"Error processing buffer stream creating response:"} + boost::diagnostic_information(error),
            request);
    }
}

MirWaitHandle* MirConnection::create_client_buffer_stream(
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    mir_buffer_stream_callback callback,
    void *context)
{
    mp::BufferStreamParameters params;
    params.set_width(width);
    params.set_height(height);
    params.set_pixel_format(format);
    params.set_buffer_usage(buffer_usage);

    auto request = std::make_shared<StreamCreationRequest>(callback, context, params);
    request->wh->expect_result();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        stream_requests.push_back(request);
    }

    try
    {
        server.create_buffer_stream(&params, request->response.get(),
            gp::NewCallback(this, &MirConnection::stream_created, request.get()));
<<<<<<< TREE
    } catch (std::exception& e)
=======
    }
    catch (std::exception const& ex)
>>>>>>> MERGE-SOURCE
    {
        //if this throws, our socket code will run the closure, which will make an error object.
        //its nicer to return an stream with a error message, so just ignore the exception.
    }

    return request->wh.get();
}

void MirConnection::stream_error(std::string const& error_msg, std::shared_ptr<StreamCreationRequest> const& request)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    mf::BufferStreamId id(next_error_id(lock).as_value());
    auto stream = std::make_shared<mcl::ErrorStream>(error_msg, this, id, request->wh);
    surface_map->insert(id, stream); 

    if (request->callback)
    {
        request->callback(reinterpret_cast<MirBufferStream*>(dynamic_cast<mcl::ClientBufferStream*>(stream.get())), request->context);
    }
    request->wh->result_received();
}

std::shared_ptr<mir::client::ClientBufferStream> MirConnection::make_consumer_stream(
   mp::BufferStream const& protobuf_bs, std::string const& surface_name, mir::geometry::Size size)
{
    return std::make_shared<mcl::BufferStream>(
        this, nullptr, server, mcl::BufferStreamMode::Consumer, platform,
        protobuf_bs, make_perf_report(logger), surface_name, size, nbuffers);
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

void MirConnection::register_lifecycle_event_callback(mir_lifecycle_event_callback callback, void* context)
{
    lifecycle_control->set_callback(std::bind(callback, this, std::placeholders::_1, context));
}

void MirConnection::register_ping_event_callback(mir_ping_event_callback callback, void* context)
{
    ping_handler->set_callback(std::bind(callback, this, std::placeholders::_1, context));
}

void MirConnection::pong(int32_t serial)
{
    mp::PingEvent pong;
    pong.set_serial(serial);
    server.pong(&pong, void_response.get(), pong_callback.get());
}

void MirConnection::register_display_change_callback(mir_display_config_callback callback, void* context)
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

std::shared_ptr<mir::logging::Logger> const& MirConnection::the_logger() const
{
    return logger;
}

MirWaitHandle* MirConnection::release_buffer_stream(
    mir::client::ClientBufferStream* stream,
    mir_buffer_stream_callback callback,
    void *context)
{
    auto new_wait_handle = new MirWaitHandle;

    StreamRelease stream_release{stream, new_wait_handle, callback, context};

    mp::BufferStreamId buffer_stream_id;
    buffer_stream_id.set_value(stream->rpc_id().as_value());

    {
        std::lock_guard<decltype(release_wait_handle_guard)> rel_lock(release_wait_handle_guard);
        release_wait_handles.push_back(new_wait_handle);
    }

    new_wait_handle->expect_result();
    server.release_buffer_stream(
        &buffer_stream_id, void_response.get(),
        google::protobuf::NewCallback(this, &MirConnection::released, stream_release));
    return new_wait_handle;
}

void MirConnection::release_consumer_stream(mir::client::ClientBufferStream* stream)
{
    surface_map->erase(stream->rpc_id());
}
