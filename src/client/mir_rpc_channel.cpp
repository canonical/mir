/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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


#include "mir_client/mir_rpc_channel.h"

#include "mir_protobuf_wire.pb.h"

#include <boost/bind.hpp>
#include <iostream>

namespace
{
// Too clever? The idea is to ensure protbuf version is verified once (on
// the first google_protobuf_guard() call) and memory is released on exit.
struct google_protobuf_guard_t
{
    google_protobuf_guard_t() { GOOGLE_PROTOBUF_VERIFY_VERSION; }
    ~google_protobuf_guard_t() { google::protobuf::ShutdownProtobufLibrary(); }
};

void google_protobuf_guard()
{
    static google_protobuf_guard_t guard;
}
}

namespace c = mir::client;
namespace cd = mir::client::detail;

cd::PendingCallCache::PendingCallCache(std::shared_ptr<Logger> const& log) :
    log(log)
{
}

cd::SendBuffer& cd::PendingCallCache::save_completion_details(
    mir::protobuf::wire::Invocation& invoke,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto& current = pending_calls[invoke.id()] = PendingCall(response, complete);
    return current.send_buffer;
}

void cd::PendingCallCache::complete_response(mir::protobuf::wire::Result& result)
{
    std::unique_lock<std::mutex> lock(mutex);
    auto call = pending_calls.find(result.id());
    if (call == pending_calls.end())
    {
        log->error() << "ERROR orphaned result: " << result.ShortDebugString() << std::endl;
    }
    else
    {
        auto& completion = call->second;
        completion.response->ParseFromString(result.response());
        completion.complete->Run();
        pending_calls.erase(call);
    }
}


c::MirRpcChannel::MirRpcChannel(std::string const& endpoint, std::shared_ptr<Logger> const& log) :
    log(log), next_message_id(0), pending_calls(log), work(io_service), endpoint(endpoint), socket(io_service)
{
    google_protobuf_guard();
    socket.connect(endpoint);

    auto run_io_service = boost::bind(&boost::asio::io_service::run, &io_service);

    for (int i = 0; i != threads; ++i)
    {
        io_service_thread[i] = std::move(std::thread(run_io_service));
    }
}

c::MirRpcChannel::~MirRpcChannel()
{
    io_service.stop();

    for (int i = 0; i != threads; ++i)
    {
        if (io_service_thread[i].joinable())
        {
            io_service_thread[i].join();
        }
    }
}


void c::MirRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController*,
    const google::protobuf::Message* parameters,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    mir::protobuf::wire::Invocation invocation = invocation_for(method, parameters);
    std::ostringstream buffer;
    invocation.SerializeToOstream(&buffer);

    // Only save details after serialization succeeds
    auto& send_buffer = pending_calls.save_completion_details(invocation, response, complete);

    // Only send message when details saved for handling response
    send_message(buffer.str(), send_buffer);
}

mir::protobuf::wire::Invocation c::MirRpcChannel::invocation_for(
    const google::protobuf::MethodDescriptor* method,
    const google::protobuf::Message* request)
{
    std::ostringstream buffer;
    request->SerializeToOstream(&buffer);

    mir::protobuf::wire::Invocation invoke;

    invoke.set_id(next_id());
    invoke.set_method_name(method->name());
    invoke.set_parameters(buffer.str());

    return invoke;
}

int c::MirRpcChannel::next_id()
{
    int id = next_message_id.load();
    while (!next_message_id.compare_exchange_weak(id, id + 1))
        id = next_message_id.load();
    return id;
}

void c::MirRpcChannel::send_message(const std::string& body, detail::SendBuffer& send_buffer)
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
        boost::bind(&MirRpcChannel::on_message_sent, this,
            boost::asio::placeholders::error));
}

void c::MirRpcChannel::on_message_sent(boost::system::error_code const& error)
{
    if (error)
    {
        log->error() << "ERROR: " << error.message() << std::endl;
        return;
    }

    read_message();
}


void c::MirRpcChannel::read_message()
{
    const size_t body_size = read_message_header();

    mir::protobuf::wire::Result result = read_message_body(body_size);

    std::cerr << "DEBUG: " << __PRETTY_FUNCTION__ << " result.id():" << result.id() << std::endl;

    pending_calls.complete_response(result);
}

size_t c::MirRpcChannel::read_message_header()
{
    unsigned char header_bytes[2];
    boost::system::error_code error;
    boost::asio::read(socket, boost::asio::buffer(header_bytes), boost::asio::transfer_exactly(sizeof header_bytes), error);
    if (error)
        log->error() << "ERROR: " << error.message() << std::endl;

    const size_t body_size = (header_bytes[0] << 8) + header_bytes[1];
    return body_size;
}

mir::protobuf::wire::Result c::MirRpcChannel::read_message_body(const size_t body_size)
{
    boost::system::error_code error;
    boost::asio::streambuf message;
    boost::asio::read(socket, message, boost::asio::transfer_at_least(body_size), error);
    if (error)
        log->error() << "ERROR: " << error.message() << std::endl;

    std::istream in(&message);
    mir::protobuf::wire::Result result;
    result.ParseFromIstream(&in);
    return result;
}


std::ostream& c::ConsoleLogger::error()
{
    return std::cerr;
}
