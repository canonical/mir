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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_screencast.h"
#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "src/server/frontend/protobuf_ipc_factory.h"
#include "mir_protobuf.pb.h"

#include "mir_test_doubles/null_display_changer.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mt = mir::test;

namespace
{

ACTION(RunClosure)
{
    arg3->Run();
}

MATCHER_P(WithOutputId, value, "")
{
    return arg->output_id() == value;
}

MATCHER_P(WithScreencastId, value, "")
{
    return arg->value() == value;
}

ACTION_P(SetCreateScreencastId, screencast_id)
{
    arg2->mutable_screencast_id()->set_value(screencast_id);
}

ACTION(FillCreateScreencastBuffer)
{
    int fds[2]{-1,-1};
    if (pipe(fds))
        FAIL() << "Failed to create FillCreateScreencastBuffer fds";
    close(fds[1]);
    arg2->mutable_buffer()->add_fd(fds[0]);
}

/*
 * This setup is used temporarily to test MirScreencast, until
 * we have the server-side implementation of the feature in place.
 */
struct MockScreencastServer : mir::protobuf::DisplayServer
{
    MockScreencastServer(
        std::shared_ptr<mir::protobuf::DisplayServer> const& ds)
        : wrapped_display_server{ds}
    {
        using namespace testing;
        ON_CALL(*this, create_screencast(_,_,_,_))
            .WillByDefault(DoAll(FillCreateScreencastBuffer(),RunClosure()));
        ON_CALL(*this, release_screencast(_,_,_,_))
            .WillByDefault(RunClosure());
    }

    void connect(
        ::google::protobuf::RpcController* rpc,
        const ::mir::protobuf::ConnectParameters* request,
        ::mir::protobuf::Connection* response,
        ::google::protobuf::Closure* done) override
    {
        wrapped_display_server->connect(rpc, request, response, done);
    }

    void disconnect(
        google::protobuf::RpcController* rpc,
        const mir::protobuf::Void* request,
        mir::protobuf::Void* response,
        google::protobuf::Closure* done) override
    {
        wrapped_display_server->disconnect(rpc, request, response, done);
    }

    MOCK_METHOD4(create_screencast,
                 void(google::protobuf::RpcController*,
                      const mir::protobuf::ScreencastParameters*,
                      mir::protobuf::Screencast*,
                      google::protobuf::Closure*));

    MOCK_METHOD4(release_screencast,
                 void(google::protobuf::RpcController*,
                      const mir::protobuf::ScreencastId*,
                      mir::protobuf::Void*,
                      google::protobuf::Closure*));

    std::shared_ptr<mir::protobuf::DisplayServer> const wrapped_display_server;
};

std::shared_ptr<MockScreencastServer> global_mock_screencast_server;

class WrappingIpcFactory : public mf::ProtobufIpcFactory
{
public:
    WrappingIpcFactory(
        std::shared_ptr<mf::ProtobufIpcFactory> const& ipc_factory)
        : wrapped_ipc_factory{ipc_factory}
    {
    }

    std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server(
        std::shared_ptr<mf::EventSink> const& sink, bool auth) override
    {
        auto ipc_server = wrapped_ipc_factory->make_ipc_server(sink, auth);
        global_mock_screencast_server =
            std::make_shared<testing::NiceMock<MockScreencastServer>>(ipc_server);
        return global_mock_screencast_server;
    }

    std::shared_ptr<mf::ResourceCache> resource_cache() override
    {
        return wrapped_ipc_factory->resource_cache();
    }

private:
    std::shared_ptr<mf::ProtobufIpcFactory> const wrapped_ipc_factory;
};

class StubChanger : public mtd::NullDisplayChanger
{
public:
    StubChanger()
        : stub_display_config{{{connected, !used}, {connected, used}}}
    {
    }

    std::shared_ptr<mg::DisplayConfiguration> active_configuration() override
    {
        return mt::fake_shared(stub_display_config);
    }

    mtd::StubDisplayConfig stub_display_config;

private:
    static bool const connected;
    static bool const used;
};

bool const StubChanger::connected{true};
bool const StubChanger::used{true};

struct StubServerConfig : mir_test_framework::StubbedServerConfiguration
{
    std::shared_ptr<mf::DisplayChanger> the_frontend_display_changer() override
    {
        return mt::fake_shared(stub_changer);
    }

    std::shared_ptr<mf::ProtobufIpcFactory> the_ipc_factory(
        std::shared_ptr<mf::Shell> const& shell,
        std::shared_ptr<mg::GraphicBufferAllocator> const& alloc) override
    {
        auto factory = mtf::StubbedServerConfiguration::the_ipc_factory(shell, alloc);
        return std::make_shared<WrappingIpcFactory>(factory);
    }

    StubChanger stub_changer;
};

class MirScreencastTest : public mir_test_framework::InProcessServer
{
public:
    ~MirScreencastTest()
    {
        global_mock_screencast_server.reset();
    }

    mir::DefaultServerConfiguration& server_config() override { return server_config_; }

    StubServerConfig server_config_;
    mtf::UsingStubClientPlatform using_stub_client_platform;
};

}

TEST_F(MirScreencastTest, creation_with_invalid_connection_fails)
{
    using namespace testing;

    uint32_t const output_id{2};
    MirScreencastParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    auto screencast = mir_connection_create_screencast_sync(nullptr, &params);
    ASSERT_EQ(nullptr, screencast);
}

TEST_F(MirScreencastTest, creation_with_invalid_output_fails)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const invalid_output_id{33};
    MirScreencastParameters params{invalid_output_id, 0, 0, mir_pixel_format_invalid};

    EXPECT_CALL(*global_mock_screencast_server,
                create_screencast(_,_,_,_))
        .Times(0);

    auto screencast =
        mir_connection_create_screencast_sync(connection, &params);
    ASSERT_EQ(nullptr, screencast);

    mir_connection_release(connection);
}

TEST_F(MirScreencastTest, create_and_release_contact_server)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const screencast_id{99};
    uint32_t const output_id{2};
    MirScreencastParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    InSequence seq;

    EXPECT_CALL(*global_mock_screencast_server,
                create_screencast(_,WithOutputId(output_id),_,_))
        .WillOnce(DoAll(SetCreateScreencastId(screencast_id),
                        FillCreateScreencastBuffer(),
                        RunClosure()));

    EXPECT_CALL(*global_mock_screencast_server,
                release_screencast(_,WithScreencastId(screencast_id),_,_))
        .WillOnce(RunClosure());

    auto screencast = mir_connection_create_screencast_sync(connection, &params);
    ASSERT_NE(nullptr, screencast);
    mir_screencast_release_sync(screencast);

    mir_connection_release(connection);
}

TEST_F(MirScreencastTest, gets_valid_egl_native_window)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const output_id{2};
    MirScreencastParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    auto screencast = mir_connection_create_screencast_sync(connection, &params);
    ASSERT_NE(nullptr, screencast);

    auto egl_native_window = mir_screencast_egl_native_window(screencast);
    EXPECT_NE(MirEGLNativeWindowType(), egl_native_window);

    mir_screencast_release_sync(screencast);

    mir_connection_release(connection);
}
