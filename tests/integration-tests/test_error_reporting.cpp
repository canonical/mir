/*
 * Copyright © 2012-2015 Canonical Ltd.
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
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_framework/deferred_in_process_server.h"

#include "mir/test/doubles/stub_ipc_factory.h"
#include "mir/test/doubles/stub_display_server.h"
#include "src/server/frontend/display_server.h"
#include "src/server/frontend/protobuf_ipc_factory.h"
#include "src/server/frontend/resource_cache.h"
#include "src/server/scene/surface_allocator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir/test/gmock_fixes.h"
#include "mir/test/validity_matchers.h"

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;

namespace
{

std::string const test_exception_text{"test exception text"};

struct ConnectionErrorServer : mtd::StubDisplayServer
{
    void connect(
        mir::protobuf::ConnectParameters const*,
        mir::protobuf::Connection*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void disconnect(
        mir::protobuf::Void const*,
        mir::protobuf::Void*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }
};

struct ErrorReporting : mtf::DeferredInProcessServer
{
    std::unique_ptr<mir::DefaultServerConfiguration> server_configuration;

    void start_server_with_config(std::unique_ptr<mir::DefaultServerConfiguration> config)
    {
        server_configuration = std::move(config);
        start_server();
    }

    mir::DefaultServerConfiguration& server_config() override
    {
        return *server_configuration;
    }
};

}

TEST_F(ErrorReporting, c_api_returns_connection_error)
{
    struct ServerConfig : mtf::TestingServerConfiguration
    {
        std::shared_ptr<mf::ProtobufIpcFactory> new_ipc_factory(
            std::shared_ptr<mf::SessionAuthorizer> const&) override
        {
            static auto error_server = std::make_shared<ConnectionErrorServer>();
            return std::make_shared<mtd::StubIpcFactory>(*error_server);
        }
    };

    start_server_with_config(std::make_unique<ServerConfig>());

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_TRUE(connection != NULL);
    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), testing::HasSubstr("Failed to connect"));

    mir_connection_release(connection);
}

TEST_F(ErrorReporting, c_api_returns_surface_creation_error)
{
    struct ServerConfig : mtf::TestingServerConfiguration
    {
        class BoobytrappedSurfaceFactory : public mir::scene::SurfaceFactory
        {
            std::shared_ptr<mir::scene::Surface>
            create_surface(
                std::shared_ptr<mir::scene::Session> const&,
                std::list<mir::scene::StreamInfo> const&,
                mir::scene::SurfaceCreationParameters const&) override
            {
                throw std::runtime_error{test_exception_text};
            }
        };

        std::shared_ptr<mir::scene::SurfaceFactory> the_surface_factory() override
        {
            return std::make_shared<BoobytrappedSurfaceFactory>();
        }
    } server_config;

    start_server_with_config(std::make_unique<ServerConfig>());

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_THAT(connection, IsValid());

    auto const spec = mir_create_normal_window_spec(connection, 640, 480);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
#pragma GCC diagnostic pop
    auto const window  = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_TRUE(window != NULL);
    ASSERT_THAT(window, Not(IsValid()));
    EXPECT_THAT(mir_window_get_error_message(window), testing::HasSubstr(test_exception_text));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}
