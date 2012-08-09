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

#include <iostream>

namespace c = mir::client;
namespace cd = mir::client::detail;


void cd::PendingCallCache::save_completion_details(
    mir::protobuf::wire::Invocation& invoke,
    google::protobuf::Message* response,
    google::protobuf::Closure* complete)
{
    std::unique_lock<std::mutex> lock(mutex);
    pending_calls[invoke.id()] = PendingCall(response, complete);
}

void cd::PendingCallCache::complete_response(mir::protobuf::wire::Result& result)
{
    std::unique_lock<std::mutex> lock(mutex);
    auto call = pending_calls.find(result.id());
    if (call == pending_calls.end())
    {
        std::cerr << "ERROR orphaned result: " << result.ShortDebugString() << std::endl;
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
    log(log), next_message_id(0), endpoint(endpoint), socket(io_service)
{
    socket.connect(endpoint);
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
    pending_calls.save_completion_details(invocation, response, complete);

    // Only send message when details saved for handling response
    send_message(buffer.str());

    read_message();
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

void c::MirRpcChannel::send_message(const std::string& body)
{
    const size_t size = body.size();
    const unsigned char header_bytes[2] =
    {
        static_cast<unsigned char>((size >> 8) & 0xff),
        static_cast<unsigned char>((size >> 0) & 0xff)
    };

    std::vector<char> message(sizeof header_bytes + size);
    std::copy(header_bytes, header_bytes + sizeof header_bytes, message.begin());
    std::copy(body.begin(), body.end(), message.begin() + sizeof header_bytes);

    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(message), error);
    if (error)
        std::cerr << "ERROR: " << error.message() << std::endl;
}


void c::MirRpcChannel::read_message()
{
    const size_t body_size = read_message_header();

    mir::protobuf::wire::Result result = read_message_body(body_size);

    pending_calls.complete_response(result);
}

size_t c::MirRpcChannel::read_message_header()
{
    unsigned char header_bytes[2];
    boost::system::error_code error;
    boost::asio::read(socket, boost::asio::buffer(header_bytes), boost::asio::transfer_exactly(sizeof header_bytes), error);
    if (error)
        std::cerr << "ERROR: " << error.message() << std::endl;

    const size_t body_size = (header_bytes[0] << 8) + header_bytes[1];
    return body_size;
}

mir::protobuf::wire::Result c::MirRpcChannel::read_message_body(const size_t body_size)
{
    boost::system::error_code error;
    boost::asio::streambuf message;
    boost::asio::read(socket, message, boost::asio::transfer_at_least(body_size), error);
    if (error)
        std::cerr << "ERROR: " << error.message() << std::endl;

    std::istream in(&message);
    mir::protobuf::wire::Result result;
    result.ParseFromIstream(&in);
    return result;
}

void c::done()
{
}

std::ostream& c::ConsoleLogger::error()
{
    return std::cerr;
}
