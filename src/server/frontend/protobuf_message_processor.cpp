/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir_toolkit/event.h"
#include "protobuf_message_processor.h"
#include "mir/frontend/message_processor_report.h"
#include "resource_cache.h"
#include "mir/frontend/client_constants.h"

#include <boost/exception/diagnostic_information.hpp>

#include <sstream>

namespace mfd = mir::frontend::detail;

mfd::ProtobufMessageProcessor::ProtobufMessageProcessor(
    std::shared_ptr<MessageSender> const& sender,
    std::shared_ptr<protobuf::DisplayServer> const& display_server,
    std::shared_ptr<ResourceCache> const& resource_cache,
    std::shared_ptr<MessageProcessorReport> const& report) :
    sender(sender),
    display_server(display_server),
    resource_cache(resource_cache),
    report(report)
{
    send_response_buffer.reserve(serialization_buffer_size);
}

template<class ResultMessage>
void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, ResultMessage* response)
{
    send_response(id, static_cast<google::protobuf::Message*>(response));
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Buffer* response)
{
    const auto& fd = extract_fds_from(response);
    send_response(id, response, {fd});
    resource_cache->free_resource(response);
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Connection* response)
{
    const auto& fd = response->has_platform() ?
        extract_fds_from(response->mutable_platform()) :
        std::vector<int32_t>();

    send_response(id, response, {fd});
    resource_cache->free_resource(response);
}

void mfd::ProtobufMessageProcessor::send_response(::google::protobuf::uint32 id, mir::protobuf::Surface* response)
{
    auto const& surface_fd = extract_fds_from(response);
    const auto& buffer_fd = response->has_buffer() ?
        extract_fds_from(response->mutable_buffer()) :
        std::vector<int32_t>();

    send_response(id, response, {surface_fd, buffer_fd});;
    resource_cache->free_resource(response);
}

template<class Response>
std::vector<int32_t> mfd::ProtobufMessageProcessor::extract_fds_from(Response* response)
{
    std::vector<int32_t> fd(response->fd().data(), response->fd().data() + response->fd().size());
    response->clear_fd();
    response->set_fds_on_side_channel(fd.size());
    return fd;
}

template<class ParameterMessage, class ResultMessage>
void mfd::ProtobufMessageProcessor::invoke(
    void (protobuf::DisplayServer::*function)(
        ::google::protobuf::RpcController* controller,
        const ParameterMessage* request,
        ResultMessage* response,
        ::google::protobuf::Closure* done),
    mir::protobuf::wire::Invocation const& invocation)
{
    ParameterMessage parameter_message;
    parameter_message.ParseFromString(invocation.parameters());
    ResultMessage result_message;

    try
    {
        std::unique_ptr<google::protobuf::Closure> callback(
            google::protobuf::NewPermanentCallback(this,
                &ProtobufMessageProcessor::send_response,
                invocation.id(),
                &result_message));

        (display_server.get()->*function)(
            0,
            &parameter_message,
            &result_message,
            callback.get());
    }
    catch (std::exception const& x)
    {
        result_message.set_error(boost::diagnostic_information(x));
        send_response(invocation.id(), &result_message);
    }
}

void mfd::ProtobufMessageProcessor::send_response(
    ::google::protobuf::uint32 id,
    google::protobuf::Message* response)
{
    send_response(id, response, FdSets());
}

void mfd::ProtobufMessageProcessor::send_response(
    ::google::protobuf::uint32 id,
    google::protobuf::Message* response,
    FdSets const& fd_sets)
{
    response->SerializeToString(&send_response_buffer);

    send_response_result.set_id(id);
    send_response_result.set_response(send_response_buffer);

    send_response_result.SerializeToString(&send_response_buffer);

    sender->send(send_response_buffer, fd_sets);
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
        else if ("drm_auth_magic" == invocation.method_name())
        {
            invoke(&protobuf::DisplayServer::drm_auth_magic, invocation);
        }
        else if ("configure_display" == invocation.method_name())
        {
            invoke(&protobuf::DisplayServer::configure_display, invocation);
        }
        else if ("configure_surface" == invocation.method_name())
        {
            invoke(&protobuf::DisplayServer::configure_surface, invocation);
        }
        else if ("disconnect" == invocation.method_name())
        {
            invoke(&protobuf::DisplayServer::disconnect, invocation);
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


bool mfd::ProtobufMessageProcessor::process_message(std::istream& msg)
{
    try
    {
        mir::protobuf::wire::Invocation invocation;
        invocation.ParseFromIstream(&msg);

        if (invocation.has_protocol_version() && invocation.protocol_version() != 1)
            BOOST_THROW_EXCEPTION(std::runtime_error("Unsupported protocol version"));

        return dispatch(invocation);
    }
    catch (std::exception const& error)
    {
        report->exception_handled(display_server.get(), error);
        return false;
    }
}
