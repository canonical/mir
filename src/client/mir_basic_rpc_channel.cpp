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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#include "mir_basic_rpc_channel.h"

#include "mir_protobuf_wire.pb.h"

#include <thread>
#include <boost/bind.hpp>
#include <sstream>

namespace mcl = mir::client;
namespace mcld = mir::client::detail;

mcld::PendingCallCache::PendingCallCache(std::shared_ptr<Logger> const& log) :
    log(log)
{
}

mcld::SendBuffer& mcld::PendingCallCache::save_completion_details(
    mir::protobuf::wire::Invocation& invoke,
    google::protobuf::Message* response,
    std::shared_ptr<google::protobuf::Closure> const& complete)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto& current = pending_calls[invoke.id()] = PendingCall(response, complete);
    log->debug() << "save_completion_details " << invoke.id() << " response " << response << " complete " << complete << std::endl;
    return current.send_buffer;
}

void mcld::PendingCallCache::complete_response(mir::protobuf::wire::Result& result)
{
    std::unique_lock<std::mutex> lock(mutex);
    log->debug() << "complete_response for result " << result.id() << std::endl;
    auto call = pending_calls.find(result.id());
    if (call == pending_calls.end())
    {
        log->error() << "orphaned result: " << result.ShortDebugString() << std::endl;
    }
    else
    {
        auto& completion = call->second;
        log->debug() << "complete_response for result " << result.id() << " response " << completion.response << " complete " << completion.complete << std::endl;
        completion.response->ParseFromString(result.response());
        completion.complete->Run();
        pending_calls.erase(call);
    }
}

bool mcld::PendingCallCache::empty() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return pending_calls.empty();
}



mcl::MirBasicRpcChannel::MirBasicRpcChannel() :
    next_message_id(0)
{
}

mcl::MirBasicRpcChannel::~MirBasicRpcChannel()
{
}


mir::protobuf::wire::Invocation mcl::MirBasicRpcChannel::invocation_for(
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

int mcl::MirBasicRpcChannel::next_id()
{
    int id = next_message_id.load();
    while (!next_message_id.compare_exchange_weak(id, id + 1)) std::this_thread::yield();
    return id;
}
