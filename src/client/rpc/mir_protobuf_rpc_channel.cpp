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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_protobuf_rpc_channel.h"
#include "rpc_report.h"
#include "mir/input/input_receiver_report.h"

#include "mir/client/surface_map.h"
#include "../buffer.h"
#include "../presentation_chain.h"
#include "../buffer_factory.h"
#include "../mir_surface.h"
#include "../display_configuration.h"
#include "../lifecycle_control.h"
#include "../event_sink.h"
#include "../make_protobuf_object.h"
#include "../protobuf_to_native_buffer.h"
#include "../mir_error.h"
#include "mir/input/input_devices.h"
#include "mir/variable_length_array.h"
#include "mir/events/event_builders.h"
#include "mir/events/event_private.h"
#include "mir/events/surface_placement_event.h"

#include "mir_protobuf.pb.h"  // For Buffer frig
#include "mir_protobuf_wire.pb.h"

#include "mir/event_printer.h"
#include <boost/bind.hpp>
#include <boost/throw_exception.hpp>
#include <endian.h>

#include <stdexcept>
#include <cstring>

namespace mf = mir::frontend;
namespace mev = mir::events;
namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
namespace md = mir::dispatch;
namespace mp = mir::protobuf;

mclr::MirProtobufRpcChannel::MirProtobufRpcChannel(
    std::unique_ptr<mclr::StreamTransport> transport,
    std::shared_ptr<mcl::SurfaceMap> const& surface_map,
    std::shared_ptr<mcl::AsyncBufferFactory> const& buffer_factory,
    std::shared_ptr<DisplayConfiguration> const& disp_config,
    std::shared_ptr<input::InputDevices> const& input_devices,
    std::shared_ptr<RpcReport> const& rpc_report,
    std::shared_ptr<input::receiver::InputReceiverReport> const& input_report,
    std::shared_ptr<LifecycleControl> const& lifecycle_control,
    std::shared_ptr<PingHandler> const& ping_handler,
    std::shared_ptr<ErrorHandler> const& error_handler,
    std::shared_ptr<EventSink> const& event_sink) :
    rpc_report(rpc_report),
    pending_calls(rpc_report),
    input_report(input_report),
    surface_map(surface_map),
    buffer_factory(buffer_factory),
    display_configuration(disp_config),
    input_devices(input_devices),
    lifecycle_control(lifecycle_control),
    ping_handler{ping_handler},
    error_handler{error_handler},
    event_sink(event_sink),
    disconnected(false),
    transport{std::move(transport)},
    delayed_processor{std::make_shared<md::ActionQueue>()},
    multiplexer{this->transport, delayed_processor}
{
    class NullDeleter
    {
    public:
        void operator()(mclr::MirProtobufRpcChannel*)
        {
        }
    };

    // This fake shared ptr is safe; we own the Transport, so the lifetime of this
    // is guaranteed to exceed the lifetime of the Transport
    this->transport->register_observer(std::shared_ptr<mclr::StreamTransport::Observer>{this, NullDeleter()});
}

void mclr::MirProtobufRpcChannel::notify_disconnected()
{
    if (!disconnected.exchange(true))
    {
        (*lifecycle_control)(mir_lifecycle_connection_lost);
    }
    pending_calls.force_completion();
    //NB: once the old semantics are not around, this explicit call to notify 
    //the streams of disconnection shouldn't be needed.
    if (auto map = surface_map.lock()) 
    {
        map->with_all_streams_do(
            [](MirBufferStream* receiver) {
                if (receiver) receiver->buffer_unavailable();
            });
    }
}

template<class MessageType>
void mclr::MirProtobufRpcChannel::receive_any_file_descriptors_for(MessageType* response)
{
    std::array<char, 1> dummy;
    if (response)
    {
        response->clear_fd();

        if (response->fds_on_side_channel() > 0)
        {
            std::vector<mir::Fd> fds(response->fds_on_side_channel());
            transport->receive_data(dummy.data(), dummy.size(), fds);
            for (auto &fd: fds)
                response->add_fd(fd);

            rpc_report->file_descriptors_received(*response, fds);
        }
        response->clear_fds_on_side_channel();
    }
}

void mclr::MirProtobufRpcChannel::receive_file_descriptors(google::protobuf::MessageLite* response)
{
    auto const message_type = response->GetTypeName();

    mir::protobuf::Surface* surface = nullptr;
    mir::protobuf::Buffer* buffer = nullptr;
    mir::protobuf::Platform* platform = nullptr;
    mir::protobuf::SocketFD* socket_fd = nullptr;
    mir::protobuf::PlatformOperationMessage* platform_operation_message = nullptr;

    if (message_type == "mir.protobuf.Buffer")
    {
        buffer = static_cast<mir::protobuf::Buffer*>(response);
    }
    else if (message_type == "mir.protobuf.BufferStream")
    {
        auto buffer_stream = static_cast<mir::protobuf::BufferStream*>(response);
        if (buffer_stream && buffer_stream->has_buffer())
            buffer = buffer_stream->mutable_buffer();
    }
    else if (message_type == "mir.protobuf.Surface")
    {
        surface = static_cast<mir::protobuf::Surface*>(response);
        if (surface && surface->has_buffer_stream() && surface->buffer_stream().has_buffer())
            buffer = surface->mutable_buffer_stream()->mutable_buffer();
    }
    else if (message_type == "mir.protobuf.Screencast")
    {
        auto screencast = static_cast<mir::protobuf::Screencast*>(response);
        if (screencast && screencast->has_buffer_stream() && screencast->buffer_stream().has_buffer())
            buffer = screencast->mutable_buffer_stream()->mutable_buffer();
    }
    else if (message_type == "mir.protobuf.Platform")
    {
        platform = static_cast<mir::protobuf::Platform*>(response);
    }
    else if (message_type == "mir.protobuf.Connection")
    {
        auto connection = static_cast<mir::protobuf::Connection*>(response);
        if (connection && connection->has_platform())
            platform = connection->mutable_platform();
    }
    else if (message_type == "mir.protobuf.SocketFD")
    {
        socket_fd = static_cast<mir::protobuf::SocketFD*>(response);
    }
    else if (message_type == "mir.protobuf.PlatformOperationMessage")
    {
        platform_operation_message =
            static_cast<mir::protobuf::PlatformOperationMessage*>(response);
    }

    receive_any_file_descriptors_for(surface);
    receive_any_file_descriptors_for(buffer);
    receive_any_file_descriptors_for(platform);
    receive_any_file_descriptors_for(socket_fd);
    receive_any_file_descriptors_for(platform_operation_message);
}

void mclr::MirProtobufRpcChannel::call_method(
    std::string const& method_name,
    google::protobuf::MessageLite const* parameters,
    google::protobuf::MessageLite* response,
    google::protobuf::Closure* complete)
{
    std::lock_guard<std::mutex> lk(discard_mutex);
    if (discard)
    {
       /*
        * Until recently we had no explicit plan for what to do in this case.
        * Callbacks would race with destruction of the MirConnection and either
        * succeed, deadlock, crash or corrupt memory. However the one apparent
        * intent in the old plan was that we close all closures so that the
        * user doesn't leak any memory. So do that...
        */
       if (complete)
           complete->Run();
       return;
    }

    if (method_name == "disconnect")
        discard = true;

    // Only send message when details saved for handling response
    std::vector<mir::Fd> fds;
    if (parameters->GetTypeName() == "mir.protobuf.BufferRequest")
    {
        auto const* buffer = reinterpret_cast<mir::protobuf::BufferRequest const*>(parameters);
        for (auto& fd : buffer->buffer().fd())
            fds.emplace_back(mir::Fd{IntOwnedFd{fd}});
    }
    else if (parameters->GetTypeName() == "mir.protobuf.PlatformOperationMessage")
    {
        auto const* request =
            reinterpret_cast<mir::protobuf::PlatformOperationMessage const*>(parameters);
        for (auto& fd : request->fd())
            fds.emplace_back(mir::Fd{IntOwnedFd{fd}});
    }

    auto const& invocation = invocation_for(method_name, parameters, fds.size());

    rpc_report->invocation_requested(invocation);

    pending_calls.save_completion_details(invocation, response, complete);

    if (prioritise_next_request)
    {
        id_to_wait_for = invocation.id();
        prioritise_next_request = false;
    }

    send_message(invocation, invocation, fds);
}

void mclr::MirProtobufRpcChannel::discard_future_calls()
{
    std::unique_lock<decltype(discard_mutex)> lk(discard_mutex);
    discard = true;
}

void mclr::MirProtobufRpcChannel::wait_for_outstanding_calls()
{
    pending_calls.wait_till_complete();
}

void mclr::MirProtobufRpcChannel::send_message(
    mir::protobuf::wire::Invocation const& body,
    mir::protobuf::wire::Invocation const& invocation,
    std::vector<mir::Fd>& fds)
{
    const size_t size = body.ByteSize();
    const unsigned char header_bytes[2] =
    {
        static_cast<unsigned char>((size >> 8) & 0xff),
        static_cast<unsigned char>((size >> 0) & 0xff)
    };

    detail::SendBuffer send_buffer(sizeof header_bytes + size);
    std::copy(header_bytes, header_bytes + sizeof header_bytes, send_buffer.begin());
    body.SerializeToArray(send_buffer.data() + sizeof header_bytes, size);

    try
    {
        std::lock_guard<decltype(write_mutex)> lock(write_mutex);
        transport->send_message(send_buffer, fds);
    }
    catch (std::runtime_error const& err)
    {
        rpc_report->invocation_failed(invocation, err);
        notify_disconnected();
        throw;
    }
    rpc_report->invocation_succeeded(invocation);
}

void mclr::MirProtobufRpcChannel::process_event_sequence(std::string const& event)
{
    mp::EventSequence seq;

    seq.ParseFromString(event);

    if (seq.has_display_configuration())
    {
        display_configuration->update_configuration(seq.display_configuration());
    }

    if (seq.has_input_configuration())
    {
        input_devices->update_devices(seq.input_configuration());
    }

    if (seq.has_lifecycle_event())
    {
        (*lifecycle_control)(static_cast<MirLifecycleState>(seq.lifecycle_event().new_state()));
    }

    if (seq.has_ping_event())
    {
        (*ping_handler)(seq.ping_event().serial());
    }

    if (seq.has_structured_error())
    {
        auto const error = MirError{
            static_cast<MirErrorDomain>(seq.structured_error().domain()),
            seq.structured_error().code()
        };
        (*error_handler)(&error);
    }

    if (seq.has_buffer_request())
    {
        std::array<char, 1> dummy;
        auto const num_fds = seq.mutable_buffer_request()->mutable_buffer()->fds_on_side_channel();
        std::vector<mir::Fd> fds(num_fds);
        if (num_fds > 0)
        {
            transport->receive_data(dummy.data(), dummy.size(), fds);
            seq.mutable_buffer_request()->mutable_buffer()->clear_fd();
            for(auto& fd : fds)
                seq.mutable_buffer_request()->mutable_buffer()->add_fd(fd);
        }

        if (auto map = surface_map.lock())
        {
            try
            {
                if (seq.buffer_request().has_id())
                {
                    mf::BufferStreamId stream_id(seq.buffer_request().id().value());
                    if (auto receiver = map->stream(stream_id))
                        receiver->buffer_available(seq.buffer_request().buffer());
                }
                
                else if (seq.buffer_request().has_operation())
                {
                    auto stream_cmd = seq.buffer_request().operation();
                    auto buffer_id = seq.buffer_request().buffer().buffer_id();
                    std::shared_ptr<mcl::MirBuffer> buffer;
                    switch (stream_cmd)
                    {
                    case mp::BufferOperation::add:
                        buffer = buffer_factory->generate_buffer(seq.buffer_request().buffer());
                        map->insert(buffer_id, buffer); 
                        buffer->received();
                        break;
                    case mp::BufferOperation::update:
                        buffer = map->buffer(buffer_id);
                        if (buffer)
                        {
                            buffer->received(
                                *mcl::protobuf_to_native_buffer(seq.buffer_request().buffer()));
                        }
                        break;
                    case mp::BufferOperation::remove:
                        /* The server never sends us an unsolicited ::remove request
                         * (and clients have no way of dealing with one)
                         *
                         * Just ignore it, because we've already deleted our buffer.
                         */
                        break;
                    default:
                        BOOST_THROW_EXCEPTION(std::runtime_error("unknown buffer operation"));
                    }
                }
            }
            catch (std::exception& e)
            {
                for(auto i = 0; i < seq.buffer_request().buffer().fd_size(); i++)
                    close(seq.buffer_request().buffer().fd(i));
                throw e;
            }
        }
        else
        {
            for(auto i = 0; i < seq.buffer_request().buffer().fd_size(); i++)
                close(seq.buffer_request().buffer().fd(i));
        }

    }

    int const nevents = seq.event_size();
    for (int i = 0; i != nevents; ++i)
    {
        mp::Event const& event = seq.event(i);
        if (event.has_raw())
        {
            // In future, events might be compressed where possible.
            // But that's a job for later...
            try
            {
                auto e = MirEvent::deserialize(event.raw());
                if (e)
                {
                    rpc_report->event_parsing_succeeded(*e);

                    int window_id = 0;
                    bool is_window_event = true;

                    switch (e->type())
                    {
                    case mir_event_type_window:
                        window_id = e->to_surface()->id();
                        break;
                    case mir_event_type_resize:
                        window_id = e->to_resize()->surface_id();
                        break;
                    case mir_event_type_orientation:
                        window_id = e->to_orientation()->surface_id();
                        break;
                    case mir_event_type_close_window:
                        window_id = e->to_close_window()->surface_id();
                        break;
                    case mir_event_type_keymap:
                        input_report->received_event(*e);
                        window_id = e->to_keymap()->surface_id();
                        break;
                    case mir_event_type_window_output:
                        window_id = e->to_window_output()->surface_id();
                        break;
                    case mir_event_type_window_placement:
                        window_id = e->to_window_placement()->id();
                        break;
                    case mir_event_type_input:
                        input_report->received_event(*e);
                        window_id = e->to_input()->window_id();
                        break;
                    case mir_event_type_input_device_state:
                        input_report->received_event(*e);
                        window_id = e->to_input_device_state()->window_id();
                        break;
                    default:
                        is_window_event = false;
                        event_sink->handle_event(*e);
                    }

                    if (is_window_event)
                        if (auto map = surface_map.lock())
                            if (auto surf = map->surface(mf::SurfaceId(window_id)))
                                surf->handle_event(*e);

                }
            }
            catch(...)
            {
                rpc_report->event_parsing_failed(event);
            }
        }
    }
}

void mclr::MirProtobufRpcChannel::on_data_available()
{
    /*
     * Our transport isn't atomic, and even if it were we don't
     * read messages from it atomically. We therefore need to guard
     * these transport->receive_data calls with a lock.
     *
     * Additionally, event processing may itself read, as that's
     * how we handle messages with file descriptors.
     *
     * So we need to lock the whole shebang
     */
    std::lock_guard<decltype(read_mutex)> lock(read_mutex);

    auto result = mcl::make_protobuf_object<mp::wire::Result>();
    try
    {
        uint16_t message_size;
        transport->receive_data(&message_size, sizeof(uint16_t));
        message_size = be16toh(message_size);

        body_bytes.resize(message_size);
        transport->receive_data(body_bytes.data(), message_size);

        result->ParseFromArray(body_bytes.data(), message_size);

        rpc_report->result_receipt_succeeded(*result);
    }
    catch (std::exception const& x)
    {
        rpc_report->result_receipt_failed(x);
        throw;
    }

    try
    {
        for (int i = 0; i != result->events_size(); ++i)
        {
            process_event_sequence(result->events(i));
        }

        if (result->has_id())
        {
            pending_calls.populate_message_for_result(
                *result,
                [&](google::protobuf::MessageLite* result_message)
                    {
                        result_message->ParseFromString(result->response());
                        receive_file_descriptors(result_message);
                    });

            if (id_to_wait_for)
            {
                if (result->id() == id_to_wait_for.value())
                {
                    pending_calls.complete_response(*result);
                    multiplexer.add_watch(delayed_processor);
                }
                else
                {
                    // It's too difficult to convince C++ to move this lambda everywhere, so
                    // just give up and let it pretend its a shared_ptr.
                    std::shared_ptr<mp::wire::Result> appeaser{std::move(result)};
                    delayed_processor->enqueue([delayed_result = std::move(appeaser), this]() mutable
                    {
                        pending_calls.complete_response(*delayed_result);
                    });
                }
            }
            else
            {
                pending_calls.complete_response(*result);
            }
        }
    }
    catch (std::exception const& x)
    {
        // TODO: This is dangerous as an error in result processing could cause a wait handle
        // to never fire. Could perhaps fix by catching and setting error on the response before invoking
        // callback ~racarr
        rpc_report->result_processing_failed(*result, x);
    }
}

void mclr::MirProtobufRpcChannel::on_disconnected()
{
    notify_disconnected();
}

mir::Fd mir::client::rpc::MirProtobufRpcChannel::watch_fd() const
{
    return multiplexer.watch_fd();
}

bool mir::client::rpc::MirProtobufRpcChannel::dispatch(md::FdEvents events)
{
    return multiplexer.dispatch(events);
}

md::FdEvents mclr::MirProtobufRpcChannel::relevant_events() const
{
    return multiplexer.relevant_events();
}

void mclr::MirProtobufRpcChannel::process_next_request_first()
{
    prioritise_next_request = true;
    multiplexer.remove_watch(delayed_processor);
}
