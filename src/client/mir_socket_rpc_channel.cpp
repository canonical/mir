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

#include "mir/protobuf/google_protobuf_guard.h"

#include "mir_protobuf.pb.h"  // For Buffer frig
#include "mir_protobuf_wire.pb.h"

#include "ancillary.h"

#include <boost/bind.hpp>
#include <cstring>

namespace mcl = mir::client;
namespace mcld = mir::client::detail;

mcl::MirSocketRpcChannel::MirSocketRpcChannel() :
    pending_calls(std::shared_ptr<Logger>()), work(io_service), socket(io_service)
{
}

mcl::MirSocketRpcChannel::MirSocketRpcChannel(
    std::string const& endpoint,
    std::shared_ptr<Logger> const& log) :
    log(log),
    pending_calls(log),
    work(io_service),
    endpoint(endpoint),
    socket(io_service),
    event_handler(nullptr)
{
    socket.connect(endpoint);

    auto run_io_service = boost::bind(&boost::asio::io_service::run, &io_service);

    io_service_thread = std::move(std::thread(run_io_service));

    boost::asio::async_read(
        socket,
        boost::asio::buffer(header_bytes),
        boost::asio::transfer_exactly(sizeof header_bytes),
        boost::bind(&MirSocketRpcChannel::on_header_read, this,
            boost::asio::placeholders::error));
}

mcl::MirSocketRpcChannel::~MirSocketRpcChannel()
{
    io_service.stop();

    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }
}

// TODO: This function needs some work.
void mcl::MirSocketRpcChannel::receive_file_descriptors(google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    log->debug() << __PRETTY_FUNCTION__ << std::endl;

    auto surface = dynamic_cast<mir::protobuf::Surface*>(response);
    if (surface)
    {
        surface->clear_fd();

        if (surface->fds_on_side_channel() > 0)
        {
            log->debug() << __PRETTY_FUNCTION__ << " expect " << surface->fds_on_side_channel() << " file descriptors" << std::endl;

            std::vector<int32_t> buf(surface->fds_on_side_channel());

            int received = 0;
            while ((received = ancil_recv_fds(socket.native_handle(), buf.data(), buf.size())) == -1)
                /* TODO avoid spinning forever */;

            log->debug() << __PRETTY_FUNCTION__ << " received " << received << " file descriptors" << std::endl;

            for (int i = 0; i != received; ++i)
                surface->add_fd(buf[i]);
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
            log->debug() << __PRETTY_FUNCTION__ << " expect " << buffer->fds_on_side_channel() << " file descriptors" << std::endl;

            std::vector<int32_t> buf(buffer->fds_on_side_channel());

            int received = 0;
            while ((received = ancil_recv_fds(socket.native_handle(), buf.data(), buf.size())) == -1)
                /* TODO avoid spinning forever */;

            log->debug() << __PRETTY_FUNCTION__ << " received " << received << " file descriptors" << std::endl;

            for (int i = 0; i != received; ++i)
                buffer->add_fd(buf[i]);
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
            log->debug() << __PRETTY_FUNCTION__ << " expect " << platform->fds_on_side_channel() << " file descriptors" << std::endl;

            std::vector<int32_t> buf(platform->fds_on_side_channel());

            int received = 0;
            while ((received = ancil_recv_fds(socket.native_handle(), buf.data(), buf.size())) == -1)
                /* TODO avoid spinning forever */;

            log->debug() << __PRETTY_FUNCTION__ << " received " << received << " file descriptors" << std::endl;

            for (int i = 0; i != received; ++i)
                platform->add_fd(buf[i]);
        }
    }


    complete->Run();
}

void mcl::MirSocketRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController*,
    const google::protobuf::Message* parameters,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    mir::protobuf::wire::Invocation invocation = invocation_for(method, parameters);
    std::ostringstream buffer;
    invocation.SerializeToOstream(&buffer);

    std::shared_ptr<google::protobuf::Closure> callback(
        google::protobuf::NewPermanentCallback(this, &MirSocketRpcChannel::receive_file_descriptors, response, complete));

    // Only save details after serialization succeeds
    auto& send_buffer = pending_calls.save_completion_details(invocation, response, callback);

    // Only send message when details saved for handling response
    send_message(buffer.str(), send_buffer);
}

void mcl::MirSocketRpcChannel::send_message(const std::string& body, detail::SendBuffer& send_buffer)
{
    const size_t size = body.size();
    const unsigned char header_bytes[2] =
    {
        static_cast<unsigned char>((size >> 8) & 0xff),
        static_cast<unsigned char>((size >> 0) & 0xff)
    };

    send_buffer.resize(sizeof header_bytes + size);
    std::copy(header_bytes, header_bytes + sizeof header_bytes, send_buffer.begin());
    std::copy(body.begin(), body.end(), send_buffer.begin() + sizeof header_bytes);

    boost::asio::async_write(
        socket,
        boost::asio::buffer(send_buffer),
        boost::bind(&MirSocketRpcChannel::on_message_sent, this,
            boost::asio::placeholders::error));
}

void mcl::MirSocketRpcChannel::on_message_sent(boost::system::error_code const& error)
{
    log->debug() << __PRETTY_FUNCTION__ << std::endl;

    if (error)
    {
        log->error() << __PRETTY_FUNCTION__
            << "\n... " << error.message() << std::endl;
    }
}

void mcl::MirSocketRpcChannel::on_header_read(const boost::system::error_code& error)
{
    log->debug() << __PRETTY_FUNCTION__ << std::endl;

    if (error)
    {
        // If we've not got a response to a call pending
        // then during shutdown we expect to see "eof"
        if (!pending_calls.empty() || error != boost::asio::error::eof)
        {
            log->error() << __PRETTY_FUNCTION__
                << "\n... " << error.message() << std::endl;
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


void mcl::MirSocketRpcChannel::read_message()
{
    try
    {
        log->debug() << __PRETTY_FUNCTION__ << std::endl;
        const size_t body_size = read_message_header();

        log->debug() << __PRETTY_FUNCTION__ << " body_size:" << body_size << std::endl;

        mir::protobuf::wire::Result result = read_message_body(body_size);

        log->debug() << __PRETTY_FUNCTION__ << " result.id():" << result.id() << std::endl;

        if (!result.has_id())  // It's an event sequence
        {
            process_event_sequence(result);
        }
        else
        {
            pending_calls.complete_response(result);
        }
    }
    catch (std::exception const& x)
    {
        log->error() << __PRETTY_FUNCTION__
            << "\n... " << x.what() << std::endl;
    }
}

void mcl::MirSocketRpcChannel::process_event_sequence(
    mir::protobuf::wire::Result const& result)
{
    if (!event_handler)
        return;

    mir::protobuf::EventSequence seq;
    if (seq.ParseFromString(result.response()))
    {
        int const nevents = seq.event_size();
        for (int i = 0; i < nevents; i++)
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
                    event_handler->handle_event(e);
                }
                else
                {
                    log->error() << __PRETTY_FUNCTION__
                                 << " Received MirEvent of an unexpected size."
                                 << std::endl;
                }
            }
        }
    } // else protobuf will log an error
}

size_t mcl::MirSocketRpcChannel::read_message_header()
{
    const size_t body_size = (header_bytes[0] << 8) + header_bytes[1];
    return body_size;
}

mir::protobuf::wire::Result mcl::MirSocketRpcChannel::read_message_body(const size_t body_size)
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

void mcl::MirSocketRpcChannel::set_event_handler(mir::EventSink *sink)
{
    /*
     * Yes, these have to be regular pointers. Because ownership of the object
     * (which is actually a MirConnection) is the responsibility of the calling
     * client. So out of our control.
     */
    event_handler = sink;
}
