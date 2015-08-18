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

#include "test_protobuf.pb.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir/client/private.h"
#include "mir/frontend/protobuf_message_sender.h"
#include "mir/frontend/protobuf_connection_creator.h"
#include "mir/frontend/template_protobuf_message_processor.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir/test/doubles/null_platform_ipc_operations.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace mtf = mir_test_framework;

/*************************************************************************/
/*************************************************************************/
/*  Note that the functionality demonstrated here relies on "detail" and */
/*  is not guaranteed to be supported in future.                         */
/*************************************************************************/
/*************************************************************************/
namespace
{
struct DemoMirServer
{
    MOCK_CONST_METHOD1(on_call, std::string(std::string));

    DemoMirServer()
    {
        using namespace  testing;
        ON_CALL(*this, on_call(_)).WillByDefault(Return("ok"));
    }

    void function(
        mir::protobuf::Parameters const* parameters,
        mir::protobuf::Result* response,
        google::protobuf::Closure* done)
    {
        response->set_value(on_call(parameters->name()));
        done->Run();
    }
};

struct AMirServer
{
    AMirServer(std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> const& channel)
        : channel{channel}
    {
    }

    void function(
        mir::protobuf::Parameters const* parameters,
        mir::protobuf::Result* response,
        google::protobuf::Closure* done)
    {
        channel->call_method(std::string(__func__), parameters, response, done);
    }
    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> channel;
};

// using a global for easy access from tests and DemoMessageProcessor::dispatch()
DemoMirServer* demo_mir_server;

struct DemoMessageProcessor : mfd::MessageProcessor
{
    DemoMessageProcessor(
        std::shared_ptr<mfd::ProtobufMessageSender> const& sender,
        std::shared_ptr<mfd::MessageProcessor> const& wrapped) :
        sender(sender),
        wrapped(wrapped) {}

    void client_pid(int /*pid*/) override {}

    bool dispatch(mfd::Invocation const& invocation, std::vector<mir::Fd> const& fds)
    {
        if ("function" == invocation.method_name())
        {
            mfd::invoke(
                this,
                demo_mir_server,
                &DemoMirServer::function,
                invocation);
            return true;
        }

        return wrapped->dispatch(invocation, fds);
    }

    void send_response(::google::protobuf::uint32 id, ::google::protobuf::MessageLite* response)
    {
        sender->send_response(id, response, {});
    }

    std::shared_ptr<mfd::ProtobufMessageSender> const sender;
    std::shared_ptr<mfd::MessageProcessor> const wrapped;
};

struct DemoConnectionCreator : mf::ProtobufConnectionCreator
{
    using ProtobufConnectionCreator::ProtobufConnectionCreator;

    MOCK_CONST_METHOD3(create_processor,
        std::shared_ptr<mfd::MessageProcessor>(
        std::shared_ptr<mfd::ProtobufMessageSender> const& sender,
        std::shared_ptr<mfd::DisplayServer> const& display_server,
        std::shared_ptr<mf::MessageProcessorReport> const& report));

    std::shared_ptr<mfd::MessageProcessor> create_wrapped_processor(
        std::shared_ptr<mfd::ProtobufMessageSender> const& sender,
        std::shared_ptr<mfd::DisplayServer> const& display_server,
        std::shared_ptr<mf::MessageProcessorReport> const& report) const
    {
        auto const wrapped = mf::ProtobufConnectionCreator::create_processor(
            sender,
            display_server,
            report);

        return std::make_shared<DemoMessageProcessor>(sender, wrapped);
    }

    std::shared_ptr<mfd::MessageProcessor> create_unwrapped_processor(
        std::shared_ptr<mfd::ProtobufMessageSender> const& sender,
        std::shared_ptr<mfd::DisplayServer> const& display_server,
        std::shared_ptr<mf::MessageProcessorReport> const& report) const
    {
        return mf::ProtobufConnectionCreator::create_processor(
            sender,
            display_server,
            report);
    }
};

struct DemoServerConfiguration : mtf::StubbedServerConfiguration
{
    std::shared_ptr<mf::ConnectionCreator> the_connection_creator() override
    {
        return connection_creator([this]
            {
                return std::make_shared<DemoConnectionCreator>(
                    new_ipc_factory(the_session_authorizer()),
                    the_session_authorizer(),
                    std::make_shared<mir::test::doubles::NullPlatformIpcOperations>(),
                    the_message_processor_report());
            });
    }

};

struct DemoPrivateProtobuf : mtf::InProcessServer
{
    mir::DefaultServerConfiguration& server_config() override { return my_server_config; }

    DemoServerConfiguration my_server_config;
    mtf::UsingStubClientPlatform using_stub_client_platform;

    std::shared_ptr<DemoConnectionCreator> demo_connection_creator;

    void SetUp()
    {
        ::demo_mir_server = &demo_mir_server;

        mtf::InProcessServer::SetUp();
        demo_connection_creator = std::dynamic_pointer_cast<DemoConnectionCreator>(my_server_config.the_connection_creator());

        using namespace testing;
        ASSERT_THAT(demo_connection_creator, NotNull());

        ON_CALL(*demo_connection_creator, create_processor(_, _, _))
            .WillByDefault(Invoke(demo_connection_creator.get(), &DemoConnectionCreator::create_unwrapped_processor));
    }

    testing::NiceMock<DemoMirServer> demo_mir_server;
};

void callback(std::atomic<bool>* called_back) { called_back->store(true); }
char const* const nothing_returned = "Nothing returned";
}

TEST_F(DemoPrivateProtobuf, client_calls_server)
{
    using namespace testing;
    EXPECT_CALL(*demo_connection_creator, create_processor(_, _, _));

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto const rpc_channel = mir::client::the_rpc_channel(connection);

    using namespace mir::protobuf;
    using namespace google::protobuf;

    AMirServer server(rpc_channel);

    Parameters parameters;
    parameters.set_name(__PRETTY_FUNCTION__);

    Result result;
    result.set_error(nothing_returned);
    std::atomic<bool> called_back{false};

    // After the call there's a race between the client releasing the connection
    // and the server dropping the connection.
    // If the latter wins we'll invoke the client's lifecycle_event_callback.
    // As the default callback kills the process with SIGHUP, we need to
    // replace it to ensure the test can continue.
    mir_connection_set_lifecycle_event_callback(
        connection,
        [](MirConnection*, MirLifecycleState, void*){},
        nullptr);

    // Note:
    // As the default server won't recognise this call it drops the connection
    // resulting in a callback when the connection drops (but result being unchanged)
    server.function(
        &parameters,
        &result,
        NewCallback(&callback, &called_back));

    mir_connection_release(connection);

    EXPECT_TRUE(called_back);
    EXPECT_THAT(result.error(), Eq(nothing_returned));
}

TEST_F(DemoPrivateProtobuf, wrapping_message_processor)
{
    using namespace testing;
    EXPECT_CALL(*demo_connection_creator, create_processor(_, _, _))
        .Times(1)
        .WillOnce(Invoke(demo_connection_creator.get(), &DemoConnectionCreator::create_wrapped_processor));

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    mir_connection_release(connection);
}

TEST_F(DemoPrivateProtobuf, server_receives_function_call)
{
    using namespace testing;
    EXPECT_CALL(*demo_connection_creator, create_processor(_, _, _))
        .WillRepeatedly(Invoke(demo_connection_creator.get(), &DemoConnectionCreator::create_wrapped_processor));

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto const rpc_channel = mir::client::the_rpc_channel(connection);

    using namespace mir::protobuf;
    using namespace google::protobuf;

    AMirServer server(rpc_channel);

    Parameters parameters;
    Result result;
    parameters.set_name(__PRETTY_FUNCTION__);

    EXPECT_CALL(demo_mir_server, on_call(__PRETTY_FUNCTION__)).Times(1);

    server.function(&parameters, &result, NewCallback([]{}));

    mir_connection_release(connection);
}


TEST_F(DemoPrivateProtobuf, client_receives_result)
{
    using namespace testing;
    EXPECT_CALL(*demo_connection_creator, create_processor(_, _, _))
        .WillRepeatedly(Invoke(demo_connection_creator.get(), &DemoConnectionCreator::create_wrapped_processor));
    EXPECT_CALL(demo_mir_server, on_call(_)).WillRepeatedly(Return(__PRETTY_FUNCTION__));

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto const rpc_channel = mir::client::the_rpc_channel(connection);

    using namespace mir::protobuf;
    using namespace google::protobuf;

    AMirServer server(rpc_channel);

    Parameters parameters;
    Result result;
    parameters.set_name(__PRETTY_FUNCTION__);

    server.function(&parameters, &result, NewCallback([]{}));

    mir_connection_release(connection);

    EXPECT_THAT(result.has_error(), Eq(false));
    EXPECT_THAT(result.value(), Eq(__PRETTY_FUNCTION__));
}
