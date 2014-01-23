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

#include "mir_toolkit/mir_output_capture.h"
#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
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

MATCHER_P(WithCaptureId, value, "")
{
    return arg->value() == value;
}

ACTION_P(SetCreateCaptureId, capture_id)
{
    arg2->mutable_capture_id()->set_value(capture_id);
}

/*
 * This setup is used temporarily to test MirOutputCapture, until
 * we have the server-side implementation of the feature in place.
 */
struct MockOutputCaptureServer : mir::protobuf::DisplayServer
{
    MockOutputCaptureServer(
        std::shared_ptr<mir::protobuf::DisplayServer> const& ds)
        : wrapped_display_server{ds}
    {
        using namespace testing;
        ON_CALL(*this, create_capture(_,_,_,_))
            .WillByDefault(RunClosure());
        ON_CALL(*this, release_capture(_,_,_,_))
            .WillByDefault(RunClosure());
        ON_CALL(*this, capture_output(_,_,_,_))
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

    MOCK_METHOD4(create_capture,
                 void(google::protobuf::RpcController*,
                      const mir::protobuf::CaptureParameters*,
                      mir::protobuf::Capture*,
                      google::protobuf::Closure*));

    MOCK_METHOD4(release_capture,
                 void(google::protobuf::RpcController*,
                      const mir::protobuf::CaptureId*,
                      mir::protobuf::Void*,
                      google::protobuf::Closure*));

    MOCK_METHOD4(capture_output,
                 void(google::protobuf::RpcController*,
                      const mir::protobuf::CaptureId*,
                      mir::protobuf::Buffer*,
                      google::protobuf::Closure*));

    std::shared_ptr<mir::protobuf::DisplayServer> const wrapped_display_server;
};

std::shared_ptr<MockOutputCaptureServer> global_mock_output_capture_server;

class WrappingIpcFactory : public mir::frontend::ProtobufIpcFactory
{
public:
    WrappingIpcFactory(
        std::shared_ptr<mir::frontend::ProtobufIpcFactory> const& ipc_factory)
        : wrapped_ipc_factory{ipc_factory}
    {
    }

    std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server(
        std::shared_ptr<mir::frontend::EventSink> const& sink, bool auth) override
    {
        auto ipc_server = wrapped_ipc_factory->make_ipc_server(sink, auth);
        global_mock_output_capture_server =
            std::make_shared<testing::NiceMock<MockOutputCaptureServer>>(ipc_server);
        return global_mock_output_capture_server;
    }

    std::shared_ptr<mir::frontend::ResourceCache> resource_cache() override
    {
        return wrapped_ipc_factory->resource_cache();
    }

private:
    std::shared_ptr<mir::frontend::ProtobufIpcFactory> const wrapped_ipc_factory;
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

    std::shared_ptr<mir::frontend::ProtobufIpcFactory> the_ipc_factory(
        std::shared_ptr<mir::frontend::Shell> const& shell,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& alloc) override
    {
        auto factory = mtf::StubbedServerConfiguration::the_ipc_factory(shell, alloc);
        return std::make_shared<WrappingIpcFactory>(factory);
    }

    StubChanger stub_changer;
};

class MirOutputCaptureTest : public mir_test_framework::InProcessServer
{
public:
    ~MirOutputCaptureTest()
    {
        global_mock_output_capture_server.reset();
    }

    mir::DefaultServerConfiguration& server_config() override { return server_config_; }

    StubServerConfig server_config_;
};

}

TEST_F(MirOutputCaptureTest, creation_with_invalid_connection_fails)
{
    using namespace testing;

    uint32_t const output_id{2};
    MirOutputCaptureParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    auto capture = mir_connection_create_output_capture_sync(nullptr, &params);
    ASSERT_EQ(nullptr, capture);
}

TEST_F(MirOutputCaptureTest, creation_with_invalid_output_fails)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const invalid_output_id{33};
    MirOutputCaptureParameters params{invalid_output_id, 0, 0, mir_pixel_format_invalid};

    EXPECT_CALL(*global_mock_output_capture_server,
                create_capture(_,_,_,_))
        .Times(0);

    auto capture =
        mir_connection_create_output_capture_sync(connection, &params);
    ASSERT_EQ(nullptr, capture);

    mir_connection_release(connection);
}

TEST_F(MirOutputCaptureTest, create_and_release_contact_server)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const capture_id{99};
    uint32_t const output_id{2};
    MirOutputCaptureParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    InSequence seq;

    EXPECT_CALL(*global_mock_output_capture_server,
                create_capture(_,WithOutputId(output_id),_,_))
        .WillOnce(DoAll(SetCreateCaptureId(capture_id), RunClosure()));

    EXPECT_CALL(*global_mock_output_capture_server,
                release_capture(_,WithCaptureId(capture_id),_,_))
        .WillOnce(RunClosure());

    auto capture = mir_connection_create_output_capture_sync(connection, &params);
    ASSERT_NE(nullptr, capture);
    mir_output_capture_release_sync(capture);

    mir_connection_release(connection);
}

TEST_F(MirOutputCaptureTest, gets_valid_egl_native_window)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const output_id{2};
    MirOutputCaptureParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    auto capture = mir_connection_create_output_capture_sync(connection, &params);
    ASSERT_NE(nullptr, capture);

    auto egl_native_window = mir_output_capture_egl_native_window(capture);
    EXPECT_NE(MirEGLNativeWindowType(), egl_native_window);

    mir_output_capture_release_sync(capture);

    mir_connection_release(connection);
}
