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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/connector.h"
#include "src/server/frontend/resource_cache.h"

#include "mir/test/stub_server_tool.h"
#include "mir/test/test_protobuf_server.h"
#include "mir/test/doubles/stub_ipc_factory.h"
#include "mir/test/test_protobuf_client.h"

namespace mt = mir::test;

namespace mir
{
namespace test
{

struct ErrorServer : StubServerTool
{
    static std::string const test_exception_text;

    void create_surface(
        const protobuf::SurfaceParameters*,
        protobuf::Surface*,
        google::protobuf::Closure*) override
    {
        throw std::runtime_error(test_exception_text);
    }

    void release_surface(
        const protobuf::SurfaceId*,
        protobuf::Void*,
        google::protobuf::Closure*) override
    {
        throw std::runtime_error(test_exception_text);
    }


    void connect(
        const ::mir::protobuf::ConnectParameters*,
        ::mir::protobuf::Connection*,
        ::google::protobuf::Closure*) override
    {
        throw std::runtime_error(test_exception_text);
    }

    void disconnect(
        const protobuf::Void*,
        protobuf::Void*,
        google::protobuf::Closure*) override
    {
        throw std::runtime_error(test_exception_text);
    }
};

std::string const ErrorServer::test_exception_text{"test exception text"};

}
}

struct ProtobufErrorTestFixture : public ::testing::Test
{
    void SetUp()
    {
        stub_services = std::make_shared<mt::ErrorServer>();
        server = std::make_shared<mt::TestProtobufServer>("./test_error_fixture", stub_services);
        client = std::make_shared<mt::TestProtobufClient>("./test_error_fixture", 100);
        server->comm->start();
    }

    void TearDown()
    {
        server.reset();
    }

    std::shared_ptr<mt::ErrorServer> stub_services;
    std::shared_ptr<mt::TestProtobufServer> server;
    std::shared_ptr<mt::TestProtobufClient>  client;
};

TEST_F(ProtobufErrorTestFixture, connect_exception)
{
    client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    EXPECT_CALL(*client, connect_done()).Times(1);

    mir::protobuf::Connection result;
    client->display_server.connect(
        &client->connect_parameters,
        &result,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::connect_done));

    client->wait_for_connect_done();

    ASSERT_TRUE(result.has_error());
    EXPECT_NE(std::string::npos, result.error().find(stub_services->test_exception_text));
}

TEST_F(ProtobufErrorTestFixture, create_surface_exception)
{
    EXPECT_CALL(*client, create_surface_done()).Times(1);

    client->display_server.create_surface(
        &client->surface_parameters,
        &client->surface,
        google::protobuf::NewCallback(client.get(), &mt::TestProtobufClient::create_surface_done));

    client->wait_for_create_surface();

    ASSERT_TRUE(client->surface.has_error());
    EXPECT_NE(std::string::npos, client->surface.error().find(stub_services->test_exception_text));

}
