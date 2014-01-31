/*
 * Copyright © 2012, 2014 Canonical Ltd.
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

#include "protobuf_message_processor.h"
#include "mir/frontend/message_processor_report.h"
#include "mir/frontend/protobuf_message_sender.h"
#include "mir/frontend/template_protobuf_message_processor.h"

namespace mfd = mir::frontend::detail;

namespace
{
template<class Response>
std::vector<int32_t> extract_fds_from(Response* response)
{
    std::vector<int32_t> fd(response->fd().data(), response->fd().data() + response->fd().size());
    response->clear_fd();
    response->set_fds_on_side_channel(fd.size());
    return fd;
}
}

mfd::ProtobufMessageProcessor::ProtobufMessageProcessor(
    std::shared_ptr<ProtobufMessageSender> const& sender,
    std::shared_ptr<protobuf::DisplayServer> const& display_server,
    std::shared_ptr<MessageProcessorReport> const& report) :
    sender(sender),
    display_server(display_server),
    report(report)
{
}

namespace mir
{
namespace frontend
{
namespace detail
{
template<> struct result_ptr_t<::mir::protobuf::Buffer>     { typedef ::mir::protobuf::Buffer* type; };
template<> struct result_ptr_t<::mir::protobuf::Connection> { typedef ::mir::protobuf::Connection* type; };
template<> struct result_ptr_t<::mir::protobuf::Surface>    { typedef ::mir::protobuf::Surface* type; };
}
}
}

bool mfd::ProtobufMessageProcessor::dispatch(mir::protobuf::wire::Invocation const& invocation)
{
    report->received_invocation(display_server.get(), invocation.id(), invocation.method_name());

    bool result = true;

    try
    {
        // TODO comparing strings in an if-else chain isn't efficient.
        // It is probably possible to generate a Trie at compile time.
        if ("connect" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::connect, invocation);
        }
        else if ("create_surface" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::create_surface, invocation);
        }
        else if ("next_buffer" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::next_buffer, invocation);
        }
        else if ("release_surface" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::release_surface, invocation);
        }
        else if ("test_file_descriptors" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::test_file_descriptors, invocation);
        }
        else if ("drm_auth_magic" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::drm_auth_magic, invocation);
        }
        else if ("configure_display" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::configure_display, invocation);
        }
        else if ("configure_surface" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::configure_surface, invocation);
        }
        else if ("create_screencast" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::create_screencast, invocation);
        }
        else if ("screencast_buffer" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::screencast_buffer, invocation);
        }
        else if ("release_screencast" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::release_screencast, invocation);
        }
        else if ("disconnect" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::disconnect, invocation);
            result = false;
        }
        else
        {
            report->unknown_method(display_server.get(), invocation.id(), invocation.method_name());
            result = false;
        }
    }
    catch (std::exception const& error)
    {
        report->exception_handled(display_server.get(), invocation.id(), error);
        result = false;
    }

    report->completed_invocation(display_server.get(), invocation.id(), result);

    return result;
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, ::google::protobuf::Message* response)
{
    sender->send_response(id, response, {});
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Buffer* response)
{
    const auto& fd = extract_fds_from(response);
    sender->send_response(id, response, {fd});
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Connection* response)
{
    const auto& fd = response->has_platform() ?
        extract_fds_from(response->mutable_platform()) :
        std::vector<int32_t>();

    sender->send_response(id, response, {fd});
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Surface* response)
{
    auto const& surface_fd = extract_fds_from(response);
    const auto& buffer_fd = response->has_buffer() ?
        extract_fds_from(response->mutable_buffer()) :
        std::vector<int32_t>();

    sender->send_response(id, response, {surface_fd, buffer_fd});
}
