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

#ifndef MIR_CLIENT_RPC_MIR_BASIC_RPC_CHANNEL_H_
#define MIR_CLIENT_RPC_MIR_BASIC_RPC_CHANNEL_H_

#include <memory>
#include <map>
#include <mutex>
#include <atomic>
#include <vector>

namespace google
{
namespace protobuf
{
class Closure;
class MessageLite;
}
}

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
namespace rpc
{

class RpcReport;

namespace detail
{
typedef std::vector<uint8_t> SendBuffer;

class PendingCallCache
{
public:
    PendingCallCache(std::shared_ptr<RpcReport> const& rpc_report);

    void save_completion_details(
        mir::protobuf::wire::Invocation const& invoke,
        google::protobuf::MessageLite* response,
        google::protobuf::Closure* complete);


    google::protobuf::MessageLite* message_for_result(mir::protobuf::wire::Result& result);

    void complete_response(mir::protobuf::wire::Result& result);

    void force_completion();

    bool empty() const;

private:

    struct PendingCall
    {
        PendingCall(
            google::protobuf::MessageLite* response,
            google::protobuf::Closure* target)
        : response(response), complete(target) {}

        PendingCall()
        : response(0), complete() {}

        google::protobuf::MessageLite* response;
        google::protobuf::Closure* complete;
    };

    std::mutex mutable mutex;
    std::map<int, PendingCall> pending_calls;
    std::shared_ptr<RpcReport> const rpc_report;
};
}

class MirBasicRpcChannel
{
public:
    virtual ~MirBasicRpcChannel();

    virtual void call_method(
        std::string const& method_name,
        google::protobuf::MessageLite const* parameters,
        google::protobuf::MessageLite* response,
        google::protobuf::Closure* complete) = 0;

protected:
    MirBasicRpcChannel();
    mir::protobuf::wire::Invocation invocation_for(
        std::string const& method_name,
        google::protobuf::MessageLite const* request,
        size_t num_side_channel_fds);
    int next_id();

private:
    std::atomic<int> next_message_id;
};

}
}
}

#endif /* MIR_CLIENT_RPC_MIR_BASIC_RPC_CHANNEL_H_ */
