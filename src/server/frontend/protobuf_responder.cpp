/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

namespace mfd = mir::frontend::detail;

mfd::ProtobufResponder::ProtobufResponder(
    std::shared_ptr<MessageSender> const& sender,
    std::shared_ptr<ResourceCache> const& resource_cache) :
    sender(sender),
    resource_cache(resource_cache)
{
    send_response_buffer.reserve(serialization_buffer_size);
}

void mfd::ProtobufResponder::send_response(
    ::google::protobuf::uint32 id,
    google::protobuf::Message* response,
    FdSets const& fd_sets)
{
    response->SerializeToString(&send_response_buffer);

    send_response_result.set_id(id);
    send_response_result.set_response(send_response_buffer);

    send_response_result.SerializeToString(&send_response_buffer);

    sender->send(send_response_buffer, fd_sets);
    resource_cache->free_resource(response);
}
