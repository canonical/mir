/*
 * Copyright © 2012. 2014 Canonical Ltd.
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


#ifndef MIR_FRONTEND_PROTOBUF_MESSAGE_PROCESSOR_H_
#define MIR_FRONTEND_PROTOBUF_MESSAGE_PROCESSOR_H_

#include "message_processor.h"

#include "mir_protobuf_wire.pb.h"
#include "mir_protobuf.pb.h"

#include <boost/exception/diagnostic_information.hpp>

#include <memory>

namespace mir
{
namespace protobuf { class DisplayServer; }

namespace frontend
{
class MessageProcessorReport;
class ProtobufMessageSender;

namespace detail
{
class TemplateProtobufMessageProcessor : public MessageProcessor
{
public:
    TemplateProtobufMessageProcessor(
        std::shared_ptr<ProtobufMessageSender> const& sender);

    ~TemplateProtobufMessageProcessor() noexcept {}

    void send_response(::google::protobuf::uint32 id, ::google::protobuf::Message* response);

    std::shared_ptr<ProtobufMessageSender> const sender;

private:
    virtual bool dispatch(mir::protobuf::wire::Invocation const& invocation) = 0;

    bool process_message(std::istream& msg) override final;
};

template<typename ResultType> struct result_ptr_t           { typedef ::google::protobuf::Message* type; };

// Utility function to allow invoke() to pick the right send_response() overload
template<typename ResultType> inline
auto result_ptr(ResultType& result) -> typename result_ptr_t<ResultType>::type { return &result; }

template<class Self, class Server, class ParameterMessage, class ResultMessage>
void invoke(
    Self* self,
    Server* server,
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
            google::protobuf::NewPermanentCallback(self,
                &Self::send_response,
                invocation.id(),
                result_ptr(result_message)));

        (server->*function)(
            0,
            &parameter_message,
            &result_message,
            callback.get());
    }
    catch (std::exception const& x)
    {
        result_message.set_error(boost::diagnostic_information(x));
        self->send_response(invocation.id(), &result_message);
    }
}


class ProtobufMessageProcessor : public TemplateProtobufMessageProcessor
{
public:
    ProtobufMessageProcessor(
        std::shared_ptr<ProtobufMessageSender> const& sender,
        std::shared_ptr<protobuf::DisplayServer> const& display_server,
        std::shared_ptr<MessageProcessorReport> const& report);

    ~ProtobufMessageProcessor() noexcept {}

    using TemplateProtobufMessageProcessor::send_response;
    void send_response(::google::protobuf::uint32 id, protobuf::Buffer* response);
    void send_response(::google::protobuf::uint32 id, protobuf::Connection* response);
    void send_response(::google::protobuf::uint32 id, protobuf::Surface* response);

private:
    bool dispatch(mir::protobuf::wire::Invocation const& invocation);

    std::shared_ptr<protobuf::DisplayServer> const display_server;
    std::shared_ptr<MessageProcessorReport> const report;
};
}
}
}

#endif /* PROTOBUF_MESSAGE_PROCESSOR_H_ */
