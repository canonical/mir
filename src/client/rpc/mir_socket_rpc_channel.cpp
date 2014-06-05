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

#include "mir_socket_rpc_channel.h"
#include "rpc_report.h"

#include "mir/protobuf/google_protobuf_guard.h"
#include "../surface_map.h"
#include "../mir_surface.h"
#include "../display_configuration.h"
#include "../lifecycle_control.h"
#include "mir/variable_length_array.h"

#include "mir_protobuf.pb.h"  // For Buffer frig
#include "mir_protobuf_wire.pb.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>
#include <boost/bind.hpp>

#include <stdexcept>

#include <signal.h>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;

namespace
{
std::chrono::milliseconds const timeout(200);
}

mclr::MirSocketRpcChannel::MirSocketRpcChannel(
    std::string const& endpoint,
    std::shared_ptr<mcl::SurfaceMap> const& surface_map,
    std::shared_ptr<DisplayConfiguration> const& disp_config,
    std::shared_ptr<RpcReport> const& rpc_report,
    std::shared_ptr<LifecycleControl> const& lifecycle_control) :
    rpc_report(rpc_report),
    pending_calls(rpc_report),
    work(io_service),
    socket(io_service),
    surface_map(surface_map),
    display_configuration(disp_config),
    lifecycle_control(lifecycle_control),
    disconnected(false)
{
    socket.connect(endpoint);
    init();
}

mclr::MirSocketRpcChannel::MirSocketRpcChannel(
    int native_socket,
    std::shared_ptr<mcl::SurfaceMap> const& surface_map,
    std::shared_ptr<DisplayConfiguration> const& disp_config,
    std::shared_ptr<RpcReport> const& rpc_report,
    std::shared_ptr<LifecycleControl> const& lifecycle_control) :
    rpc_report(rpc_report),
    pending_calls(rpc_report),
    work(io_service),
    socket(io_service),
    surface_map(surface_map),
    display_configuration(disp_config),
    lifecycle_control(lifecycle_control),
    disconnected(false)
{
    socket.assign(boost::asio::local::stream_protocol(), native_socket);
    init();
}

void mclr::MirSocketRpcChannel::notify_disconnected()
{
    if (!disconnected.exchange(true))
    {
        io_service.stop();
        socket.close();
        lifecycle_control->call_lifecycle_event_handler(mir_lifecycle_connection_lost);
    }
    pending_calls.force_completion();
}

void mclr::MirSocketRpcChannel::init()
{
    io_service_thread = std::thread([this]
        {
            // Our IO threads must not receive any signals
            sigset_t all_signals;
            sigfillset(&all_signals);

            if (auto error = pthread_sigmask(SIG_BLOCK, &all_signals, NULL))
                BOOST_THROW_EXCEPTION(
                    boost::enable_error_info(
                        std::runtime_error("Failed to block signals on IO thread")) << boost::errinfo_errno(error));

            boost::asio::async_read(
                socket,
                boost::asio::buffer(header_bytes),
                boost::asio::transfer_exactly(sizeof header_bytes),
                boost::bind(&MirSocketRpcChannel::on_header_read, this,
                    boost::asio::placeholders::error));

            try
            {
                io_service.run();
            }
            catch (std::exception const& x)
            {
                rpc_report->connection_failure(x);

                notify_disconnected();
            }
        });
}

mclr::MirSocketRpcChannel::~MirSocketRpcChannel()
{
    io_service.stop();

    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }
}

template<class MessageType>
void mclr::MirSocketRpcChannel::receive_any_file_descriptors_for(MessageType* response)
{
    if (response)
    {
        response->clear_fd();

        if (response->fds_on_side_channel() > 0)
        {
            std::vector<int32_t> fds(response->fds_on_side_channel());
            receive_file_descriptors(fds);
            for (auto &fd: fds)
                response->add_fd(fd);

            rpc_report->file_descriptors_received(*response, fds);
        }
        response->clear_fds_on_side_channel();
    }
}

void mclr::MirSocketRpcChannel::receive_file_descriptors(google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    if (!disconnected.load())
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
    }

    complete->Run();
}

void mclr::MirSocketRpcChannel::receive_file_descriptors(std::vector<int>& fds)
{
    // We send dummy data
    struct iovec iov;
    char dummy_iov_data = '\0';
    iov.iov_base = &dummy_iov_data;
    iov.iov_len = 1;

    // Allocate space for control message
    static auto const builtin_n_fds = 5;
    static auto const builtin_cmsg_space = CMSG_SPACE(builtin_n_fds * sizeof(int));
    auto const fds_bytes = fds.size() * sizeof(int);
    mir::VariableLengthArray<builtin_cmsg_space> control{CMSG_SPACE(fds_bytes)};

    // Message to send
    struct msghdr header;
    header.msg_name = NULL;
    header.msg_namelen = 0;
    header.msg_iov = &iov;
    header.msg_iovlen = 1;
    header.msg_controllen = control.size();
    header.msg_control = control.data();
    header.msg_flags = 0;

    using std::chrono::steady_clock;
    auto const time_limit = steady_clock::now() + timeout;

    while (recvmsg(socket.native_handle(), &header, 0) < 0)
    {
        if (steady_clock::now() > time_limit)
            return;
    }

    // If we get a proper control message, copy the received
    // file descriptors back to the caller
    struct cmsghdr const* const cmsg = CMSG_FIRSTHDR(&header);

    if (cmsg && cmsg->cmsg_len == CMSG_LEN(fds_bytes) &&
        cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
    {
        int const* const data = reinterpret_cast<int const*>CMSG_DATA(cmsg);
        int i = 0;
        for (auto& fd : fds)
            fd = data[i++];
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Invalid control message for receiving file descriptors"));
    }
}

void mclr::MirSocketRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController*,
    const google::protobuf::Message* parameters,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    auto const& invocation = invocation_for(method, parameters);

    rpc_report->invocation_requested(invocation);

    std::shared_ptr<google::protobuf::Closure> callback(
        google::protobuf::NewPermanentCallback(this, &MirSocketRpcChannel::receive_file_descriptors, response, complete));

    // Only save details after serialization succeeds
    pending_calls.save_completion_details(invocation, response, callback);

    // Only send message when details saved for handling response
    send_message(invocation, invocation);
}

void mclr::MirSocketRpcChannel::send_message(
    mir::protobuf::wire::Invocation const& body,
    mir::protobuf::wire::Invocation const& invocation)
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

    boost::system::error_code error;

    boost::asio::write(
        socket,
        boost::asio::buffer(send_buffer),
        error);

    if (error)
    {
        rpc_report->invocation_failed(invocation, error);
        notify_disconnected();
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to send message to server: " + error.message()));
    }

    rpc_report->invocation_succeeded(invocation);
}

void mclr::MirSocketRpcChannel::on_header_read(const boost::system::error_code& error)
{
    if (error)
    {
        // If we've not got a response to a call pending
        // then during shutdown we expect to see "eof"
        if (!pending_calls.empty() || error != boost::asio::error::eof)
        {
            rpc_report->header_receipt_failed(error);
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to read message header: " + error.message()));
        }

        return;
    }

    read_message();

    boost::asio::async_read(
        socket,
        boost::asio::buffer(header_bytes),
        boost::asio::transfer_exactly(sizeof header_bytes),
        boost::bind(&MirSocketRpcChannel::on_header_read, this,
            boost::asio::placeholders::error));
}

void mclr::MirSocketRpcChannel::read_message()
{
    mir::protobuf::wire::Result result;

    try
    {
        const size_t body_size = read_message_header();

        read_message_body(result, body_size);

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

void mclr::MirSocketRpcChannel::process_event_sequence(std::string const& event)
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

                surface_map->with_surface_do(e.surface.id,
                    [&e](MirSurface* surface)
                    {
                        surface->handle_event(e);
                    });
            }
            else
            {
                rpc_report->event_parsing_failed(event);
            }
        }
    }
}

size_t mclr::MirSocketRpcChannel::read_message_header()
{
    const size_t body_size = (header_bytes[0] << 8) + header_bytes[1];
    return body_size;
}

void mclr::MirSocketRpcChannel::read_message_body(
    mir::protobuf::wire::Result& result,
    size_t const body_size)
{
    boost::system::error_code error;
    body_bytes.resize(body_size);
    boost::asio::read(socket, boost::asio::buffer(body_bytes), error);
    if (error)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to read message body: " + error.message()));
    }

    result.ParseFromArray(body_bytes.data(), body_size);
}
