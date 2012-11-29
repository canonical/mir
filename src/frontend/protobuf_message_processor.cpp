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

#include "protobuf_message_processor.h"

#include <sstream>
#include <iostream>

namespace mfd = mir::frontend::detail;

mfd::ProtobufMessageProcessor::ProtobufMessageProcessor(
    Sender* sender,
    std::shared_ptr<protobuf::DisplayServer> const& display_server,
    std::shared_ptr<ResourceCache> const& resource_cache) :
    sender(sender),
    display_server(display_server),
    resource_cache(resource_cache)
{
}


void mfd::ProtobufMessageProcessor::send_response(
    ::google::protobuf::uint32 id,
    google::protobuf::Message* response)
{
    std::ostringstream buffer1;
    response->SerializeToOstream(&buffer1);

    mir::protobuf::wire::Result result;
    result.set_id(id);
    result.set_response(buffer1.str());

    std::ostringstream buffer2;
    result.SerializeToOstream(&buffer2);

    sender->send(buffer2);
}

bool mfd::ProtobufMessageProcessor::process_message(std::istream& msg)
{
    mir::protobuf::wire::Invocation invocation;

    invocation.ParseFromIstream(&msg);
    // TODO comparing strings in an if-else chain isn't efficient.
    // It is probably possible to generate a Trie at compile time.
    if ("connect" == invocation.method_name())
    {
        invoke(&protobuf::DisplayServer::connect, invocation);
    }
    else if ("create_surface" == invocation.method_name())
    {
        invoke(&protobuf::DisplayServer::create_surface, invocation);
    }
    else if ("next_buffer" == invocation.method_name())
    {
        invoke(&protobuf::DisplayServer::next_buffer, invocation);
    }
    else if ("release_surface" == invocation.method_name())
    {
        invoke(&protobuf::DisplayServer::release_surface, invocation);
    }
    else if ("test_file_descriptors" == invocation.method_name())
    {
        invoke(&protobuf::DisplayServer::test_file_descriptors, invocation);
    }
    else if ("disconnect" == invocation.method_name())
    {
        invoke(&protobuf::DisplayServer::disconnect, invocation);
        // Careful about what you do after this - it deletes this
        return false;
    }
    else
    {
        /*log->error()*/
        std::cerr << "Unknown method:" << invocation.method_name() << std::endl;
    }

    return true;
}
