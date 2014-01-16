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

#include "template_protobuf_message_processor.h"
#include "protobuf_message_sender.h"

namespace mfd = mir::frontend::detail;

mfd::TemplateProtobufMessageProcessor::TemplateProtobufMessageProcessor(
    std::shared_ptr<ProtobufMessageSender> const& sender) :
    sender(sender)
{
}

bool mfd::TemplateProtobufMessageProcessor::process_message(std::istream& msg)
{
    mir::protobuf::wire::Invocation invocation;
    invocation.ParseFromIstream(&msg);

    if (invocation.has_protocol_version() && invocation.protocol_version() != 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Unsupported protocol version"));

    return dispatch(invocation);
}

void mfd::TemplateProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, ::google::protobuf::Message* response)
{
    sender->send_response(id, response, {});
}
