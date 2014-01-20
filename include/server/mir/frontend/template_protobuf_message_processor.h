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


#ifndef MIR_FRONTEND_TEMPLATE_PROTOBUF_MESSAGE_PROCESSOR_H_
#define MIR_FRONTEND_TEMPLATE_PROTOBUF_MESSAGE_PROCESSOR_H_

#include "mir/frontend/message_processor.h"

#include "mir_protobuf_wire.pb.h"

#include <google/protobuf/service.h>

#include <boost/exception/diagnostic_information.hpp>

#include <memory>

namespace mir
{
namespace frontend
{
namespace detail
{
class ProtobufMessageSender;

// This class is intended to make implementation of a protobuf based MessageProcessor simpler.
// The template method process_message() calls dispatch after unpacking the received "invocation"
// message. The related invoke<>() template handles further unpacking of the parameter
// message and packing of the response and calls send_response.
// Derived classes can overload send_response, but need to specialize result_ptr_t before instantiating
// invoke<>() to ensure the correct overload is called.
class TemplateProtobufMessageProcessor : public MessageProcessor
{
public:
    TemplateProtobufMessageProcessor(
        std::shared_ptr<ProtobufMessageSender> const& sender);

    ~TemplateProtobufMessageProcessor() noexcept {}

    void send_response(::google::protobuf::uint32 id, ::google::protobuf::Message* response);

    std::shared_ptr<ProtobufMessageSender> const sender;

    virtual bool dispatch(mir::protobuf::wire::Invocation const& invocation) = 0;

private:
    bool process_message(std::istream& msg) override final;
};

// Utility metafunction result_ptr_t<> allows invoke() to pick the right
// send_response() overload. Client code may specialize result_ptr_t to
// resolve an overload of send_response called.
template<typename ResultType> struct result_ptr_t
{ typedef ::google::protobuf::Message* type; };

// Boiler plate for unpacking a parameter message, invoking a server function, and
// sending the result message.
template<class Self, class Server, class ParameterMessage, class ResultMessage>
void invoke(
    Self* self,
    Server* server,
    void (Server::*function)(
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
            google::protobuf::NewPermanentCallback<
                Self,
                ::google::protobuf::uint32,
                typename result_ptr_t<ResultMessage>::type>(
                    self,
                    &Self::send_response,
                    invocation.id(),
                    &result_message));

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
}
}
}

#endif /* MIR_FRONTEND_TEMPLATE_PROTOBUF_MESSAGE_PROCESSOR_H_ */
