/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/frontend/connector.h"
#include "src/server/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"

#include "mir/test/doubles/stub_ipc_factory.h"
#include "mir/test/stub_server_tool.h"
#include "mir/test/test_protobuf_server.h"
#include "mir/test/doubles/null_client_event_sink.h"
#include "mir_test_framework/testing_server_configuration.h"

#include "src/client/connection_surface_map.h"
#include "src/client/display_configuration.h"
#include "src/client/lifecycle_control.h"
#include "src/client/ping_handler.h"
#include "src/client/rpc/null_rpc_report.h"
#include "src/client/rpc/make_rpc_channel.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "src/client/rpc/mir_display_server.h"
#include "src/client/buffer_factory.h"
#include "mir/input/input_devices.h"
#include "mir/input/null_input_receiver_report.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <stdexcept>
#include <memory>
#include <string>
#include <atomic>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
struct StubProtobufClient
{
    StubProtobufClient(std::string socket_file, int timeout_ms);

    std::shared_ptr<mir::client::rpc::RpcReport> rpc_report;
    std::shared_ptr<mir::input::receiver::InputReceiverReport> input_report;
    std::shared_ptr<mir::client::SurfaceMap> surface_map;
    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> channel;
    mir::client::rpc::DisplayServer display_server;
    mir::protobuf::ConnectParameters connect_parameters;
    mir::protobuf::SurfaceParameters surface_parameters;
    mir::protobuf::Surface surface;
    mir::protobuf::Void ignored;

    void create_surface_done();
    void submit_buffer_done();
    void disconnect_done();

    void wait_for_create_surface();

    void wait_for_submit_buffer();

    void wait_for_disconnect_done();


    const int maxwait;
    std::atomic<bool> connect_done_called;
    std::atomic<bool> create_surface_called;
    std::atomic<bool> submit_buffer_called;
    std::atomic<bool> release_surface_called;
    std::atomic<bool> disconnect_done_called;
    std::atomic<bool> tfd_done_called;

    std::atomic<int> connect_done_count;
    std::atomic<int> create_surface_done_count;
    std::atomic<int> disconnect_done_count;
};
}

struct StressProtobufCommunicator : public ::testing::Test
{
    static void SetUpTestCase()
    {
        stub_server_tool = std::make_shared<mt::StubServerTool>();
        stub_server = std::make_shared<mt::TestProtobufServer>(mtf::test_socket_file(), stub_server_tool);

        stub_server->comm->start();
    }

    void SetUp()
    {
        client = std::make_shared<StubProtobufClient>(mtf::test_socket_file(), 100);
        client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        client.reset();
    }

    static void TearDownTestCase()
    {
        stub_server.reset();
        stub_server_tool.reset();
    }

    std::shared_ptr<StubProtobufClient> client;
    static std::shared_ptr<mt::StubServerTool> stub_server_tool;
private:
    static std::shared_ptr<mt::TestProtobufServer> stub_server;
};

std::shared_ptr<mt::StubServerTool> StressProtobufCommunicator::stub_server_tool;
std::shared_ptr<mt::TestProtobufServer> StressProtobufCommunicator::stub_server;


TEST_F(StressProtobufCommunicator, DISABLED_stress_submit_buffer)
{
    client->display_server.create_surface(
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &StubProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    mir::protobuf::Void response;
    for (int i = 0; i != 100000; ++i)
    {
        mir::protobuf::BufferRequest request;
        request.mutable_id()->set_value(client->surface.id().value());
        *request.mutable_buffer() = client->surface.buffer();
        client->display_server.submit_buffer(
            &request, &response,
            google::protobuf::NewCallback(client.get(), &StubProtobufClient::submit_buffer_done));

        client->wait_for_submit_buffer();
    }

    client->display_server.disconnect(
        &client->ignored,
        &client->ignored,
        google::protobuf::NewCallback(client.get(), &StubProtobufClient::disconnect_done));

    client->wait_for_disconnect_done();
}

StubProtobufClient::StubProtobufClient(
    std::string socket_file,
    int timeout_ms) :
    rpc_report(std::make_shared<mir::client::rpc::NullRpcReport>()),
    input_report(std::make_shared<mir::input::receiver::NullInputReceiverReport>()),
    surface_map(std::make_shared<mir::client::ConnectionSurfaceMap>()),
    channel(mir::client::rpc::make_rpc_channel(
        socket_file,
        surface_map,
        std::make_shared<mir::client::BufferFactory>(),
        std::make_shared<mir::client::DisplayConfiguration>(),
        std::make_shared<mir::input::InputDevices>(surface_map),
        rpc_report,
        input_report,
        std::make_shared<mir::client::LifecycleControl>(),
        std::make_shared<mir::client::PingHandler>(),
        std::make_shared<mir::client::ErrorHandler>(),
        std::make_shared<mtd::NullClientEventSink>())),
    display_server(channel),
    maxwait(timeout_ms),
    connect_done_called(false),
    create_surface_called(false),
    submit_buffer_called(false),
    release_surface_called(false),
    disconnect_done_called(false),
    tfd_done_called(false),
    connect_done_count(0),
    create_surface_done_count(0),
    disconnect_done_count(0)
{
    surface_parameters.set_width(640);
    surface_parameters.set_height(480);
    surface_parameters.set_pixel_format(0);
    surface_parameters.set_buffer_usage(0);
}

void StubProtobufClient::create_surface_done()
{
    create_surface_called.store(true);

    auto old = create_surface_done_count.load();

    while (!create_surface_done_count.compare_exchange_weak(old, old+1));
}

void StubProtobufClient::submit_buffer_done()
{
    submit_buffer_called.store(true);
}

void StubProtobufClient::disconnect_done()
{
    disconnect_done_called.store(true);

    auto old = disconnect_done_count.load();

    while (!disconnect_done_count.compare_exchange_weak(old, old+1));
}

void StubProtobufClient::wait_for_create_surface()
{
    for (int i = 0; !create_surface_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    create_surface_called.store(false);
}

void StubProtobufClient::wait_for_submit_buffer()
{
    for (int i = 0; !submit_buffer_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    submit_buffer_called.store(false);
}

void StubProtobufClient::wait_for_disconnect_done()
{
    for (int i = 0; !disconnect_done_called.load() && i < maxwait; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::this_thread::yield();
    }
    disconnect_done_called.store(false);
}
