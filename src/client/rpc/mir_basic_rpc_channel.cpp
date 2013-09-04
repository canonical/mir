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

#include "mir_basic_rpc_channel.h"
#include "rpc_report.h"

#include "mir_protobuf_wire.pb.h"
#include "mir/frontend/client_constants.h"

#include <sstream>

namespace mclr = mir::client::rpc;
namespace mclrd = mir::client::rpc::detail;

mclrd::PendingCallCache::PendingCallCache(
    std::shared_ptr<RpcReport> const& rpc_report)
    : rpc_report{rpc_report}
{
}

mclrd::SendBuffer& mclrd::PendingCallCache::save_completion_details(
    mir::protobuf::wire::Invocation& invoke,
    google::protobuf::Message* response,
    std::shared_ptr<google::protobuf::Closure> const& complete)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto& current = pending_calls[invoke.id()] = PendingCall(response, complete);
    return current.send_buffer;
}

void mclrd::PendingCallCache::complete_response(mir::protobuf::wire::Result& result)
{
    std::unique_lock<std::mutex> lock(mutex);
    auto call = pending_calls.find(result.id());
    if (call == pending_calls.end())
    {
        rpc_report->orphaned_result(result);
    }
    else
    {
        auto& completion = call->second;

        rpc_report->complete_response(result);

        completion.response->ParseFromString(result.response());

        // Let the completion closure live a bit longer than our lock...
        std::shared_ptr<google::protobuf::Closure> complete =
            completion.complete;
        pending_calls.erase(call);

        lock.unlock();  // Avoid deadlocks in callbacks
        complete->Run();
    }
}

bool mclrd::PendingCallCache::empty() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return pending_calls.empty();
}



mclr::MirBasicRpcChannel::MirBasicRpcChannel() :
    next_message_id(0)
{
}

mclr::MirBasicRpcChannel::~MirBasicRpcChannel()
{
}


mir::protobuf::wire::Invocation mclr::MirBasicRpcChannel::invocation_for(
    const google::protobuf::MethodDescriptor* method,
    const google::protobuf::Message* request)
{
    char buffer[mir::frontend::serialization_buffer_size];

    auto const size = request->ByteSize();
    // In practice size will be 10s of bytes - but have a test to detect problems
    assert(size < static_cast<decltype(size)>(sizeof buffer));

    request->SerializeToArray(buffer, sizeof buffer);

    mir::protobuf::wire::Invocation invoke;

    invoke.set_id(next_id());
    invoke.set_method_name(method->name());
    invoke.set_parameters(buffer, size);
    invoke.set_protocol_version(1);

    return invoke;
}

int mclr::MirBasicRpcChannel::next_id()
{
    return next_message_id.fetch_add(1);
}
