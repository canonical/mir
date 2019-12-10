/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
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

#include "protobuf_responder.h"
#include "resource_cache.h"
#include "message_sender.h"
#include "mir/frontend/client_constants.h"
#include "mir/variable_length_array.h"
#include "socket_messenger.h"

namespace mfd = mir::frontend::detail;

mfd::ProtobufResponder::ProtobufResponder(
    std::shared_ptr<MessageSender> const& sender,
    std::shared_ptr<ResourceCache> const& resource_cache) :
    sender(sender),
    resource_cache(resource_cache)
{
}

void mfd::ProtobufResponder::send_response(
    ::google::protobuf::uint32 id,
    google::protobuf::MessageLite* response,
    FdSets const& fd_sets)
{
    mir::VariableLengthArray<serialization_buffer_size>
#if GOOGLE_PROTOBUF_VERSION >= 3010000
        send_response_buffer{static_cast<size_t>(response->ByteSizeLong())};
#else
        send_response_buffer{static_cast<size_t>(response->ByteSize())};
#endif

    response->SerializeWithCachedSizesToArray(send_response_buffer.data());

    {
        std::lock_guard<decltype(result_guard)> lock{result_guard};

        send_response_result.set_id(id);
        send_response_result.set_response(send_response_buffer.data(), send_response_buffer.size());

#if GOOGLE_PROTOBUF_VERSION >= 3010000
        send_response_buffer.resize(send_response_result.ByteSizeLong());
#else
        send_response_buffer.resize(send_response_result.ByteSize());
#endif
        send_response_result.SerializeWithCachedSizesToArray(send_response_buffer.data());
    }

    sender->send(reinterpret_cast<char*>(send_response_buffer.data()), send_response_buffer.size(), fd_sets);
    resource_cache->free_resource(response);
}
