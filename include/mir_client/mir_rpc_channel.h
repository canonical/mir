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


#ifndef MIR_RPC_CHANNEL_H_
#define MIR_RPC_CHANNEL_H_

#include "mir/thread/all.h"
#include "mir_client/mir_logger.h"

#include <boost/asio.hpp>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include <iosfwd>

namespace mir
{
namespace protobuf
{
namespace wire
{
class Invocation;
class Result;
}
}

namespace client
{
namespace detail
{
typedef std::vector<char> SendBuffer;

class PendingCallCache
{
public:
    PendingCallCache(std::shared_ptr<Logger> const& log);

    SendBuffer& save_completion_details(
        mir::protobuf::wire::Invocation& invoke,
        google::protobuf::Message* response,
        google::protobuf::Closure* complete);

    void complete_response(mir::protobuf::wire::Result& result);

private:

    struct PendingCall
    {
        PendingCall(google::protobuf::Message* response, google::protobuf::Closure* target)
        : response(response), complete(target) {}

        PendingCall()
        : response(0), complete(0) {}

        SendBuffer send_buffer;
        google::protobuf::Message* response;
        google::protobuf::Closure* complete;
    };

    std::mutex mutable mutex;
    std::map<int, PendingCall> pending_calls;
    std::shared_ptr<Logger> const log;
};
}

class MirRpcChannel : public google::protobuf::RpcChannel
{
public:
    MirRpcChannel(
        std::string const& endpoint,
        std::shared_ptr<Logger> const& log);

    ~MirRpcChannel();

private:
    virtual void CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController*,
        const google::protobuf::Message* parameters,
        google::protobuf::Message* response,
        google::protobuf::Closure* complete);

    std::shared_ptr<Logger> log;
    std::atomic<int> next_message_id;

    detail::PendingCallCache pending_calls;
    static const int threads = 2;
    std::thread io_service_thread[threads];
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
    boost::asio::local::stream_protocol::endpoint endpoint;
    boost::asio::local::stream_protocol::socket socket;

    mir::protobuf::wire::Invocation invocation_for(
        const google::protobuf::MethodDescriptor* method,
        const google::protobuf::Message* request);

    int next_id();

    void send_message(const std::string& body, detail::SendBuffer& buffer);
    void on_message_sent(boost::system::error_code const& error);

    void read_message();

    size_t read_message_header();

    mir::protobuf::wire::Result read_message_body(const size_t body_size);
};

}
}



#endif /* MIR_RPC_CHANNEL_H_ */
