/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_debug.h"

#include "src/client/client_platform_factory.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/stub_client_connection_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <cstring>

namespace mcl = mir::client;
namespace mtf = mir_test_framework;

namespace
{
class ExceptionThrowingConfiguration : public mtf::StubConnectionConfiguration
{
    using mtf::StubConnectionConfiguration::StubConnectionConfiguration;

    std::shared_ptr<mir::client::ClientPlatformFactory> the_client_platform_factory() override
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Ducks!"});
    }
};

class BoobytrappedPlatformFactory : public mir::client::ClientPlatformFactory
{
    std::shared_ptr<mir::client::ClientPlatform>
    create_client_platform(mir::client::ClientContext* /*context*/) override
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Ducks!"});
    }
};

class BoobytrappedPlatformFactoryConfiguration : public mtf::StubConnectionConfiguration
{
    using mtf::StubConnectionConfiguration::StubConnectionConfiguration;

    std::shared_ptr<mir::client::ClientPlatformFactory> the_client_platform_factory() override
    {
        return std::make_shared<BoobytrappedPlatformFactory>();
    }
};


class ClientLibraryErrors : public mtf::InProcessServer
{
private:
    mir::DefaultServerConfiguration &server_config() override
    {
        return config;
    }
    mtf::StubbedServerConfiguration config;
};
}

TEST_F(ClientLibraryErrors, ExceptionInClientConfigurationConstructorGeneratesError)
{
    mtf::UsingClientPlatform<ExceptionThrowingConfiguration> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), testing::HasSubstr("Ducks!"));
}

TEST_F(ClientLibraryErrors, ExceptionInPlatformConstructionGeneratesError)
{
    mtf::UsingClientPlatform<BoobytrappedPlatformFactoryConfiguration> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), testing::HasSubstr("Ducks!"));
}

TEST_F(ClientLibraryErrors, ConnectingToGarbageSocketReturnsAppropriateError)
{
    using namespace testing;
    auto connection = mir_connect_sync("garbage", __PRETTY_FUNCTION__);
    ASSERT_THAT(connection, NotNull());

    char const* error = mir_connection_get_error_message(connection);

    if (std::strcmp("connect: No such file or directory", error) &&
        std::strcmp("Can't find MIR server", error) &&
        !std::strstr(error, "Failed to connect to server socket"))
    {
        FAIL() << error;
    }
}

using ClientLibraryErrorsDeathTest = ClientLibraryErrors;


TEST_F(ClientLibraryErrorsDeathTest, CreateingSurfaceOnGarbageConnectionIsFatal)
{
    mtf::UsingStubClientPlatform stubby;

    auto connection = mir_connect_sync("garbage", __PRETTY_FUNCTION__);

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mir_connection_create_surface_sync(connection, &request_params), "");
}


TEST_F(ClientLibraryErrorsDeathTest, CreatingSurfaceSynchronoslyOnMalconstructedConnectionIsFatal)
{
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mtf::UsingClientPlatform<ExceptionThrowingConfiguration> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mir_connection_create_surface_sync(connection, &request_params), "");
}

TEST_F(ClientLibraryErrorsDeathTest, CreatingSurfaceSynchronoslyOnInvalidConnectionIsFatal)
{
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mtf::UsingClientPlatform<BoobytrappedPlatformFactoryConfiguration> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mir_connection_create_surface_sync(connection, &request_params), "");
}
