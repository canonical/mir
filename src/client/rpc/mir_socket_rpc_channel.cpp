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

#include "mir_protobuf.pb.h"  // For Buffer frig
#include "mir_protobuf_wire.pb.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <boost/bind.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <sstream>
#include <stdexcept>

#include <pthread.h>
#include <signal.h>

#include <stdio.h>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;

mclr::MirSocketRpcChannel::MirSocketRpcChannel(
    std::string const& endpoint,
    std::shared_ptr<mcl::SurfaceMap> const& surface_map,
    std::shared_ptr<DisplayConfiguration> const& disp_config,
    std::shared_ptr<RpcReport> const& rpc_report,
    std::shared_ptr<LifecycleControl> const& lifecycle_control) :
    rpc_report(rpc_report),
    pending_calls(rpc_report),
    work(io_service),
    endpoint(endpoint),
    socket(io_service),
    surface_map(surface_map),
    display_configuration(disp_config),
    lifecycle_control(lifecycle_control)
{
    socket.connect(endpoint);

    auto run_io_service = boost::bind(&boost::asio::io_service::run, &io_service);

    // Our IO threads must not recieve any signals
    sigset_t all_signals;
    sigfillset(&all_signals);
    sigset_t old_mask;
    int error;
    if ((error = pthread_sigmask(SIG_BLOCK, &all_signals, &old_mask)))
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to block signals on IO thread")) << boost::errinfo_errno(error));
            

    io_service_thread = std::move(std::thread(run_io_service));

    // Restore previous signals.
    if ((error = pthread_sigmask(SIG_SETMASK, &old_mask, NULL)))
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to restore signal mask")) << boost::errinfo_errno(error));

    boost::asio::async_read(
        socket,
        boost::asio::buffer(header_bytes),
        boost::asio::transfer_exactly(sizeof header_bytes),
        boost::bind(&MirSocketRpcChannel::on_header_read, this,
            boost::asio::placeholders::error));
}

mclr::MirSocketRpcChannel::~MirSocketRpcChannel()
{
    io_service.stop();

    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }
}

// TODO: This function needs some work.
void mclr::MirSocketRpcChannel::receive_file_descriptors(google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    auto surface = dynamic_cast<mir::protobuf::Surface*>(response);
    if (surface)
    {
        surface->clear_fd();

        if (surface->fds_on_side_channel() > 0)
        {
            std::vector<int32_t> fds(surface->fds_on_side_channel());
            receive_file_descriptors(fds);
            for (auto &fd: fds)
                surface->add_fd(fd);

            rpc_report->file_descriptors_received(*response, fds);
        }
    }

    auto buffer = dynamic_cast<mir::protobuf::Buffer*>(response);
    if (!buffer)
    {
        if (surface && surface->has_buffer())
            buffer = surface->mutable_buffer();
    }

    if (buffer)
    {
        buffer->clear_fd();

        if (buffer->fds_on_side_channel() > 0)
        {
            std::vector<int32_t> fds(buffer->fds_on_side_channel());
            receive_file_descriptors(fds);
            for (auto &fd: fds)
                buffer->add_fd(fd);

            rpc_report->file_descriptors_received(*response, fds);
        }
    }

    auto platform = dynamic_cast<mir::protobuf::Platform*>(response);
    if (!platform)
    {
        auto connection = dynamic_cast<mir::protobuf::Connection*>(response);
        if (connection && connection->has_platform())
            platform = connection->mutable_platform();
    }

    if (platform)
    {
        platform->clear_fd();

        if (platform->fds_on_side_channel() > 0)
        {
            std::vector<int32_t> fds(platform->fds_on_side_channel());
            receive_file_descriptors(fds);
            for (auto &fd: fds)
                platform->add_fd(fd);

            rpc_report->file_descriptors_received(*response, fds);
        }
    }


    complete->Run();
}

void mclr::MirSocketRpcChannel::receive_file_descriptors(std::vector<int> &fds)
{
    // We send dummy data
    struct iovec iov;
    char dummy_iov_data = '\0';
    iov.iov_base = &dummy_iov_data;
    iov.iov_len = 1;

    // Allocate space for control message
    auto n_fds = fds.size();
    std::vector<char> control(sizeof(struct cmsghdr) + sizeof(int) * n_fds);

    // Message to send
    struct msghdr header;
    header.msg_name = NULL;
    header.msg_namelen = 0;
    header.msg_iov = &iov;
    header.msg_iovlen = 1;
    header.msg_controllen = control.size();
    header.msg_control = control.data();
    header.msg_flags = 0;

    // Control message contains file descriptors
    struct cmsghdr *message = CMSG_FIRSTHDR(&header);
    message->cmsg_len = header.msg_controllen;
    message->cmsg_level = SOL_SOCKET;
    message->cmsg_type = SCM_RIGHTS;

    while (recvmsg(socket.native_handle(), &header, 0) < 0)
        ; // FIXME: Avoid spinning forever

    // Copy file descriptors back to caller
    n_fds = (message->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
    fds.resize(n_fds);
    int *data = (int *)CMSG_DATA(message);
    for (std::vector<int>::size_type i = 0; i < n_fds; i++)
        fds[i] = data[i];
}

void mclr::MirSocketRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController*,
    const google::protobuf::Message* parameters,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    mir::protobuf::wire::Invocation invocation = invocation_for(method, parameters);

    rpc_report->invocation_requested(invocation);

    std::shared_ptr<google::protobuf::Closure> callback(
        google::protobuf::NewPermanentCallback(this, &MirSocketRpcChannel::receive_file_descriptors, response, complete));

    // Only save details after serialization succeeds
    auto& send_buffer = pending_calls.save_completion_details(invocation, response, callback);

    // Only send message when details saved for handling response
    send_message(invocation, send_buffer, invocation);
}

void mclr::MirSocketRpcChannel::send_message(
    mir::protobuf::wire::Invocation const& body,
    detail::SendBuffer& send_buffer,
    mir::protobuf::wire::Invocation const& invocation)
{
    const size_t size = body.ByteSize();
    const unsigned char header_bytes[2] =
    {
        static_cast<unsigned char>((size >> 8) & 0xff),
        static_cast<unsigned char>((size >> 0) & 0xff)
    };

    send_buffer.resize(sizeof header_bytes + size);
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
        
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to send message to server"));
    }
    else
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

        result = read_message_body(body_size);

        rpc_report->result_receipt_succeeded(result);
    }
    catch (std::exception const& x)
    {
        rpc_report->result_receipt_failed(x);
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

mir::protobuf::wire::Result mclr::MirSocketRpcChannel::read_message_body(const size_t body_size)
{
    boost::system::error_code error;
    boost::asio::streambuf message;
    boost::asio::read(socket, message, boost::asio::transfer_exactly(body_size), error);
    if (error)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }

    std::istream in(&message);
    mir::protobuf::wire::Result result;
    result.ParseFromIstream(&in);
    return result;
}
