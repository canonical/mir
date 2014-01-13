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


#ifndef MIR_FRONTEND_PROTOBUF_RESPONSE_PROCESSOR_H_
#define MIR_FRONTEND_PROTOBUF_RESPONSE_PROCESSOR_H_

#include "fd_sets.h"

#include "message_sender.h"

#include "mir_protobuf_wire.pb.h"

#include <memory>

namespace mir
{
namespace frontend
{
class ResourceCache;

namespace detail
{
class ProtobufResponseProcessor
{
public:
    ProtobufResponseProcessor(
        std::shared_ptr<MessageSender> const& sender,
        std::shared_ptr<ResourceCache> const& resource_cache);

    // Providing a template wrapper allows client code to specialize the logic
    template<class ResultMessage>
    void send_response(::google::protobuf::uint32 id, ResultMessage* response)
    {
        send_response(id, static_cast<google::protobuf::Message*>(response));
    }

private:
    ProtobufResponseProcessor(ProtobufResponseProcessor const&) = delete;
    ProtobufResponseProcessor operator=(ProtobufResponseProcessor const&) = delete;

    void send_response(::google::protobuf::uint32 id, google::protobuf::Message* response);
    void send_response(::google::protobuf::uint32 id, google::protobuf::Message* response,
        FdSets const& fd_sets);

    std::shared_ptr<MessageSender> const sender;
    std::shared_ptr<ResourceCache> const resource_cache;

    std::string send_response_buffer;
    mir::protobuf::wire::Result send_response_result;
};
}
}
}

#endif /* PROTOBUF_MESSAGE_PROCESSOR_H_ */
