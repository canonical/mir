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


#ifndef MIR_FRONTEND_PROTOBUF_MESSAGE_PROCESSOR_H_
#define MIR_FRONTEND_PROTOBUF_MESSAGE_PROCESSOR_H_

#include "mir/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"

#include <vector>
#include <memory>
#include <iosfwd>
#include <cstdint>

namespace mir
{
namespace protobuf { class DisplayServer; }

namespace frontend
{
namespace detail
{
class ProtobufMessageProcessor;

struct Sender
{
    virtual void send(const std::ostringstream& buffer2) = 0;
    virtual void send_fds(std::vector<int32_t> const& fd) = 0;
};

struct MessageProcessor
{
    virtual bool process_message(std::istream& msg) = 0;
};

struct NullMessageProcessor : MessageProcessor
{
    bool process_message(std::istream& ) { return false; }
};

struct ProtobufMessageProcessor : MessageProcessor
{
    ProtobufMessageProcessor(
        Sender* sender,
        std::shared_ptr<protobuf::DisplayServer> const& display_server,
        std::shared_ptr<ResourceCache> const& resource_cache);

    void send_response(::google::protobuf::uint32 id, google::protobuf::Message* response);

    template<class ResultMessage>
    void send_response(::google::protobuf::uint32 id, ResultMessage* response)
    {
        send_response(id, static_cast<google::protobuf::Message*>(response));
    }

    // TODO detecting the message type to see if we send FDs seems a bit of a frig.
    // OTOH until we have a real requirement it is hard to see how best to generalise.
    void send_response(::google::protobuf::uint32 id, mir::protobuf::Buffer* response)
    {
        const auto& fd = extract_fds_from(response);
        send_response(id, static_cast<google::protobuf::Message*>(response));
        sender->send_fds(fd);
        resource_cache->free_resource(response);
    }

    // TODO detecting the message type to see if we send FDs seems a bit of a frig.
    // OTOH until we have a real requirement it is hard to see how best to generalise.
    void send_response(::google::protobuf::uint32 id, mir::protobuf::Platform* response)
    {
        const auto& fd = extract_fds_from(response);
        send_response(id, static_cast<google::protobuf::Message*>(response));
        sender->send_fds(fd);
        resource_cache->free_resource(response);
    }

    // TODO detecting the message type to see if we send FDs seems a bit of a frig.
    // OTOH until we have a real requirement it is hard to see how best to generalise.
    void send_response(::google::protobuf::uint32 id, mir::protobuf::Connection* response)
    {
        const auto& fd = response->has_platform() ?
            extract_fds_from(response->mutable_platform()) :
            std::vector<int32_t>();

        send_response(id, static_cast<google::protobuf::Message*>(response));
        sender->send_fds(fd);
        resource_cache->free_resource(response);
    }

    // TODO detecting the message type to see if we send FDs seems a bit of a frig.
    // OTOH until we have a real requirement it is hard to see how best to generalise.
    void send_response(::google::protobuf::uint32 id, mir::protobuf::Surface* response)
    {
        const auto& fd = response->has_buffer() ?
            extract_fds_from(response->mutable_buffer()) :
            std::vector<int32_t>();

        send_response(id, static_cast<google::protobuf::Message*>(response));
        sender->send_fds(fd);
        resource_cache->free_resource(response);
    }

    template<class Response>
    std::vector<int32_t> extract_fds_from(Response* response)
    {
        std::vector<int32_t> fd(response->fd().data(), response->fd().data() + response->fd().size());
        response->clear_fd();
        response->set_fds_on_side_channel(fd.size());
        return fd;
    }

    bool process_message(std::istream& msg);

    template<class ParameterMessage, class ResultMessage>
    void invoke(
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
            result_message.set_error(x.what());
            send_response(invocation.id(), &result_message);
        }
    }
private:
    Sender* const sender;
    std::shared_ptr<protobuf::DisplayServer> const display_server;
    mir::protobuf::Surface surface;
    std::shared_ptr<ResourceCache> const resource_cache;
};
}
}
}



#endif /* PROTOBUF_MESSAGE_PROCESSOR_H_ */
