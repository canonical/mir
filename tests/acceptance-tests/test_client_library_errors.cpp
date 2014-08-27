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

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/stub_client_connection_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>

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

