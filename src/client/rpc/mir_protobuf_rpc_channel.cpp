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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_protobuf_rpc_channel.h"
#include "rpc_report.h"

#include "mir/protobuf/google_protobuf_guard.h"
#include "../surface_map.h"
#include "../mir_surface.h"
#include "../display_configuration.h"
#include "../lifecycle_control.h"
#include "../event_sink.h"
#include "mir/variable_length_array.h"

#include "mir_protobuf.pb.h"  // For Buffer frig
#include "mir_protobuf_wire.pb.h"

#include <boost/bind.hpp>
#include <endian.h>

#include <stdexcept>


namespace mcl = mir::client;
namespace mclr = mir::client::rpc;

namespace
{
std::chrono::milliseconds const timeout(200);
}

mclr::MirProtobufRpcChannel::MirProtobufRpcChannel(
    std::unique_ptr<mclr::StreamTransport> transport,
    std::shared_ptr<mcl::SurfaceMap> const& surface_map,
    std::shared_ptr<DisplayConfiguration> const& disp_config,
    std::shared_ptr<RpcReport> const& rpc_report,
    std::shared_ptr<LifecycleControl> const& lifecycle_control,
    std::shared_ptr<EventSink> const& event_sink) :
    rpc_report(rpc_report),
    pending_calls(rpc_report),
    surface_map(surface_map),
    display_configuration(disp_config),
    lifecycle_control(lifecycle_control),
    event_sink(event_sink),
    disconnected(false),
    transport{std::move(transport)}
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
        lifecycle_control->call_lifecycle_event_handler(mir_lifecycle_connection_lost);
    }
    pending_calls.force_completion();
}


template<class MessageType>
void mclr::MirProtobufRpcChannel::receive_any_file_descriptors_for(MessageType* response)
{
    static std::array<char, 1> dummy;
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

void mclr::MirProtobufRpcChannel::receive_file_descriptors(google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    auto const message_type = response->GetTypeName();

    mir::protobuf::Surface* surface = nullptr;
    mir::protobuf::Buffer* buffer = nullptr;
    mir::protobuf::Platform* platform = nullptr;
    mir::protobuf::SocketFD* socket_fd = nullptr;

    if (message_type == "mir.protobuf.Buffer")
    {
        buffer = static_cast<mir::protobuf::Buffer*>(response);
    }
    else if (message_type == "mir.protobuf.Surface")
    {
        surface = static_cast<mir::protobuf::Surface*>(response);
        if (surface && surface->has_buffer())
            buffer = surface->mutable_buffer();
    }
    else if (message_type == "mir.protobuf.Screencast")
    {
        auto screencast = static_cast<mir::protobuf::Screencast*>(response);
        if (screencast && screencast->has_buffer())
            buffer = screencast->mutable_buffer();
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

    receive_any_file_descriptors_for(surface);
    receive_any_file_descriptors_for(buffer);
    receive_any_file_descriptors_for(platform);
    receive_any_file_descriptors_for(socket_fd);
    complete->Run();
}

void mclr::MirProtobufRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController*,
    const google::protobuf::Message* parameters,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    // Only send message when details saved for handling response
    std::vector<mir::Fd> fds;
    if (parameters->GetTypeName() == "mir.protobuf.BufferRequest")
    {
        auto const* buffer = reinterpret_cast<mir::protobuf::BufferRequest const*>(parameters);
        for(auto& fd : buffer->buffer().fd())
            fds.emplace_back(mir::Fd{IntOwnedFd{fd}});
    }

    auto const& invocation = invocation_for(method, parameters, fds.size());

    rpc_report->invocation_requested(invocation);

    std::shared_ptr<google::protobuf::Closure> callback(
        google::protobuf::NewPermanentCallback(this, &MirProtobufRpcChannel::receive_file_descriptors, response, complete));

    // Only save details after serialization succeeds
    pending_calls.save_completion_details(invocation, response, callback);

    send_message(invocation, invocation, fds);
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
    mir::protobuf::EventSequence seq;

    seq.ParseFromString(event);

    if (seq.has_display_configuration())
    {
        display_configuration->update_configuration(seq.display_configuration());
    }

    if (seq.has_lifecycle_event())
    {
        lifecycle_control->call_lifecycle_event_handler(seq.lifecycle_event().new_state());
    }

    int const nevents = seq.event_size();
    for (int i = 0; i != nevents; ++i)
    {
        mir::protobuf::Event const& event = seq.event(i);
        if (event.has_raw())
        {
            std::string const& raw_event = event.raw();

            // In future, events might be compressed where possible.
            // But that's a job for later...
            if (raw_event.size() == sizeof(MirEvent))
            {
                MirEvent e;

                // Make a copy to ensure integer fields get correct memory
                // alignment, which is critical on many non-x86
                // architectures.
                memcpy(&e, raw_event.data(), sizeof e);

                rpc_report->event_parsing_succeeded(e);

                auto const send_e = [&e](MirSurface* surface)
                    { surface->handle_event(e); };

                switch (e.type)
                {
                case mir_event_type_surface:
                    surface_map->with_surface_do(e.surface.id, send_e);
                    break;

                case mir_event_type_resize:
                    surface_map->with_surface_do(e.resize.surface_id, send_e);
                    break;

                case mir_event_type_orientation:
                    surface_map->with_surface_do(e.orientation.surface_id, send_e);
                    break;

                default:
                    event_sink->handle_event(e);
                }
            }
            else
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

    mir::protobuf::wire::Result result;
    try
    {
        uint16_t message_size;
        transport->receive_data(&message_size, sizeof(uint16_t));
        message_size = be16toh(message_size);

        body_bytes.resize(message_size);
        transport->receive_data(body_bytes.data(), message_size);

        result.ParseFromArray(body_bytes.data(), message_size);

        rpc_report->result_receipt_succeeded(result);
    }
    catch (std::exception const& x)
    {
        rpc_report->result_receipt_failed(x);
        throw;
    }

    try
    {
        for (int i = 0; i != result.events_size(); ++i)
        {
            process_event_sequence(result.events(i));
        }

        if (result.has_id())
        {
            pending_calls.complete_response(result);
        }
    }
    catch (std::exception const& x)
    {
        rpc_report->result_processing_failed(result, x);
        // Eat this exception as it doesn't affect rpc
    }
}

void mclr::MirProtobufRpcChannel::on_disconnected()
{
    notify_disconnected();
}
