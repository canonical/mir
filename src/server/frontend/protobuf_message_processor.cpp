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

#include "display_server.h"
#include "protobuf_message_processor.h"
#include "mir/frontend/message_processor_report.h"
#include "mir/frontend/protobuf_message_sender.h"
#include "mir/frontend/template_protobuf_message_processor.h"

#include "mir_protobuf_wire.pb.h"

namespace mfd = mir::frontend::detail;

namespace
{
template<class Response>
std::vector<mir::Fd> extract_fds_from(Response* response)
{
    std::vector<mir::Fd> fd;
    for(auto i = 0; i < response->fd().size(); ++i)
        fd.emplace_back(mir::Fd(dup(response->fd().data()[i])));
    response->clear_fd();
    response->set_fds_on_side_channel(fd.size());
    return fd;
}
}

mfd::ProtobufMessageProcessor::ProtobufMessageProcessor(
    std::shared_ptr<ProtobufMessageSender> const& sender,
    std::shared_ptr<DisplayServer> const& display_server,
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
template<> struct result_ptr_t<::mir::protobuf::Screencast> { typedef ::mir::protobuf::Screencast* type; };
template<> struct result_ptr_t<mir::protobuf::SocketFD>     { typedef ::mir::protobuf::SocketFD* type; };

template<>
void invoke(
    ProtobufMessageProcessor* self,
    DisplayServer* server,
    void (mir::protobuf::DisplayServer::*function)(
        ::google::protobuf::RpcController* controller,
        const protobuf::SurfaceId* request,
        protobuf::Buffer* response,
        ::google::protobuf::Closure* done),
        Invocation const& invocation)
{
    protobuf::SurfaceId parameter_message;
    parameter_message.ParseFromString(invocation.parameters());
    auto const result_message = std::make_shared<protobuf::Buffer>();

    auto const callback =
        google::protobuf::NewCallback<
            ProtobufMessageProcessor,
            ::google::protobuf::uint32,
             std::shared_ptr<protobuf::Buffer>>(
                self,
                &ProtobufMessageProcessor::send_response,
                invocation.id(),
                result_message);

    try
    {
        (server->*function)(
            0,
            &parameter_message,
            result_message.get(),
            callback);
    }
    catch (std::exception const& x)
    {
        delete callback;
        result_message->set_error(boost::diagnostic_information(x));
        self->send_response(invocation.id(), result_message);
    }
}
}
}
}


const std::string& mfd::Invocation::method_name() const
{
    return invocation.method_name();
}

const std::string& mfd::Invocation::parameters() const
{
    return invocation.parameters();
}

google::protobuf::uint32 mfd::Invocation::id() const
{
    return invocation.id();
}

void mfd::ProtobufMessageProcessor::client_pid(int pid)
{
    display_server->client_pid(pid);
}

bool mfd::ProtobufMessageProcessor::dispatch(Invocation const& invocation)
{
    report->received_invocation(display_server.get(), invocation.id(), invocation.method_name());

    bool result = true;

    try
    {
        // TODO comparing strings in an if-else chain isn't efficient.
        // It is probably possible to generate a Trie at compile time.
        if ("connect" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::connect, invocation);
        }
        else if ("create_surface" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::create_surface, invocation);
        }
        else if ("next_buffer" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::next_buffer, invocation);
        }
        else if ("exchange_buffer" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::exchange_buffer, invocation);
        }
        else if ("release_surface" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::release_surface, invocation);
        }
        else if ("drm_auth_magic" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::drm_auth_magic, invocation);
        }
        else if ("configure_display" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::configure_display, invocation);
        }
        else if ("configure_surface" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::configure_surface, invocation);
        }
        else if ("create_screencast" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::create_screencast, invocation);
        }
        else if ("screencast_buffer" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::screencast_buffer, invocation);
        }
        else if ("release_screencast" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::release_screencast, invocation);
        }
        else if ("configure_cursor" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::configure_cursor, invocation);
        }
        else if ("new_fds_for_prompt_providers" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::new_fds_for_prompt_providers, invocation);
        }
        else if ("start_prompt_session" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::start_prompt_session, invocation);
        }
        else if ("stop_prompt_session" == invocation.method_name())
        {
            invoke(this, display_server.get(), &protobuf::DisplayServer::stop_prompt_session, invocation);
        }
        else if ("disconnect" == invocation.method_name())
        {
            invoke(this, display_server.get(), &DisplayServer::disconnect, invocation);
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
    sender->send_response(id, response, {extract_fds_from(response)});
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, std::shared_ptr<protobuf::Buffer> response)
{
    send_response(id, response.get());
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Connection* response)
{
    if (response->has_platform())
        sender->send_response(id, response, {extract_fds_from(response->mutable_platform())});
    else
        sender->send_response(id, response, {});
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Surface* response)
{
    if (response->has_buffer())
        sender->send_response(id, response, {extract_fds_from(response), extract_fds_from(response->mutable_buffer())});
    else
        sender->send_response(id, response, {extract_fds_from(response)});
}

void mfd::ProtobufMessageProcessor::send_response(
    ::google::protobuf::uint32 id, mir::protobuf::Screencast* response)
{
    if (response->has_buffer())
        sender->send_response(id, response, {extract_fds_from(response->mutable_buffer())});
    else
        sender->send_response(id, response, {});
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::SocketFD* response)
{
    sender->send_response(id, response, {extract_fds_from(response)});
}
