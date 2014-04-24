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

#include "spike_socket_request.pb.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir/client/private.h"
#include "mir/frontend/protobuf_message_sender.h"
#include "mir/frontend/protobuf_session_creator.h"
#include "mir/frontend/template_protobuf_message_processor.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;

namespace
{
struct SocketFDServer : mir::protobuf::SocketFDServer
{
    void client_socket_fd(
        ::google::protobuf::RpcController* ,
        ::mir::protobuf::SocketFDRequest const* /*parameters*/,
        ::mir::protobuf::SocketFD* /*response*/,
        ::google::protobuf::Closure* /*done*/)
    {
        throw std::runtime_error("not implemented");
//        response->
//        done->Run();
    }
};

// using a global for easy access from tests and MessageProcessor::dispatch()
SocketFDServer* demo_mir_server;

struct MessageProcessor : mfd::MessageProcessor
{
    MessageProcessor(
        std::shared_ptr<mfd::ProtobufMessageSender> const& sender,
        std::shared_ptr<mfd::MessageProcessor> const& wrapped) :
        sender(sender),
        wrapped(wrapped)
    {
    }

    bool dispatch(mfd::Invocation const& invocation)
    {
        if ("client_socket_fd" == invocation.method_name())
        {
            mfd::invoke(
                this,
                demo_mir_server,
                &SocketFDServer::client_socket_fd,
                invocation);
            return true;
        }

        return wrapped->dispatch(invocation);
    }

    void send_response(::google::protobuf::uint32 id, ::google::protobuf::Message* response)
    {
        sender->send_response(id, response, {});
    }

    std::shared_ptr<mfd::ProtobufMessageSender> const sender;
    std::shared_ptr<mfd::MessageProcessor> const wrapped;
};

struct DemoSessionCreator : mf::ProtobufSessionCreator
{
    using ProtobufSessionCreator::ProtobufSessionCreator;

    std::shared_ptr<mfd::MessageProcessor> create_processor(
        std::shared_ptr<mfd::ProtobufMessageSender> const& sender,
        std::shared_ptr<mir::protobuf::DisplayServer> const& display_server,
        std::shared_ptr<mf::MessageProcessorReport> const& report) const
    {
        auto const wrapped = mf::ProtobufSessionCreator::create_processor(
            sender,
            display_server,
            report);

        return std::make_shared<MessageProcessor>(sender, wrapped);
    }
};

struct DemoServerConfiguration : mir_test_framework::StubbedServerConfiguration
{
    std::shared_ptr<mf::SessionCreator> the_session_creator() override
    {
        return session_creator([this]
            {
                return std::make_shared<DemoSessionCreator>(
                    the_ipc_factory(the_frontend_shell(), the_buffer_allocator()),
                    the_session_authorizer(),
                    the_message_processor_report());
            });
    }
};

struct DemoSocketFDServer : mir_test_framework::InProcessServer
{
    mir::DefaultServerConfiguration& server_config() override { return my_server_config; }

    DemoServerConfiguration my_server_config;

    void SetUp()
    {
        ::demo_mir_server = &demo_mir_server;

        mir_test_framework::InProcessServer::SetUp();
    }

    testing::NiceMock<SocketFDServer> demo_mir_server;
};
}

TEST_F(DemoSocketFDServer, client_gets_fd)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto const rpc_channel = mir::client::the_rpc_channel(connection);
    SocketFDServer::Stub server(rpc_channel.get());

    using namespace mir::protobuf;
    using namespace google::protobuf;

    SocketFDRequest parameters;
    parameters.set_application_name(__PRETTY_FUNCTION__);

    SocketFD result;

    server.client_socket_fd(
        nullptr,
        &parameters,
        &result,
        NewCallback([]{}));

    mir_connection_release(connection);

    EXPECT_THAT(result.fd_size(), Eq(1));
}
