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

#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"
#include "mir/frontend/client_constants.h"
#include "mir/variable_length_array.h"

#include <sstream>

namespace mclr = mir::client::rpc;
namespace mclrd = mir::client::rpc::detail;

mclrd::PendingCallCache::PendingCallCache(
    std::shared_ptr<RpcReport> const& rpc_report)
    : rpc_report{rpc_report}
{
}

void mclrd::PendingCallCache::save_completion_details(
    mir::protobuf::wire::Invocation const& invoke,
    google::protobuf::MessageLite* response,
    google::protobuf::Closure* complete)
{
    std::unique_lock<std::mutex> lock(mutex);

    pending_calls[invoke.id()] = PendingCall(response, complete);
}

google::protobuf::MessageLite* mclrd::PendingCallCache::message_for_result(mir::protobuf::wire::Result& result)
{
    std::unique_lock<std::mutex> lock(mutex);
    return pending_calls.at(result.id()).response;
}

void mclrd::PendingCallCache::complete_response(mir::protobuf::wire::Result& result)
{
    PendingCall completion;

    {
        std::unique_lock<std::mutex> lock(mutex);
        auto call = pending_calls.find(result.id());
        if (call != pending_calls.end())
        {
            completion = call->second;
            pending_calls.erase(call);
        }
    }

    if (!completion.complete)
    {
        rpc_report->orphaned_result(result);
    }
    else
    {
        rpc_report->complete_response(result);
        completion.complete->Run();
    }
}

void mclrd::PendingCallCache::force_completion()
{
    std::unique_lock<std::mutex> lock(mutex);
    for (auto& call : pending_calls)
    {
        auto& completion = call.second;
        completion.complete->Run();
    }

    pending_calls.erase(pending_calls.begin(), pending_calls.end());
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

mclr::MirBasicRpcChannel::~MirBasicRpcChannel() = default;

mir::protobuf::wire::Invocation mclr::MirBasicRpcChannel::invocation_for(
    std::string const& method_name,
    google::protobuf::MessageLite const* request,
    size_t num_side_channel_fds)
{
    mir::VariableLengthArray<mir::frontend::serialization_buffer_size>
        buffer{static_cast<size_t>(request->ByteSize())};

    request->SerializeWithCachedSizesToArray(buffer.data());

    mir::protobuf::wire::Invocation invoke;

    invoke.set_id(next_id());
    invoke.set_method_name(method_name);
    invoke.set_parameters(buffer.data(), buffer.size());
    invoke.set_protocol_version(1);
    invoke.set_side_channel_fds(num_side_channel_fds);

    return invoke;
}

int mclr::MirBasicRpcChannel::next_id()
{
    return next_message_id.fetch_add(1);
}
