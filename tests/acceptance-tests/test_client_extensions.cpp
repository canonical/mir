/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/stub_platform_extension.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_toolkit/mir_extension_core.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
using namespace testing;

struct ClientExtensions : mtf::HeadlessInProcessServer
{
    MirConnection* connect()
    {
        auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
        if (!mir_connection_is_valid(connection))
            throw std::runtime_error("could not connect");
        return connection;
    }
};

TEST_F(ClientExtensions, can_load_an_extension)
{
    auto connection = connect();
    auto ext = mir_extension_favorite_flavor_v1(connection);
    ASSERT_THAT(ext, Ne(nullptr));
    ASSERT_THAT(ext->favorite_flavor, Ne(nullptr));
    EXPECT_THAT(ext->favorite_flavor(), StrEq("banana"));
    mir_connection_release(connection);
}

TEST_F(ClientExtensions, can_load_an_extension_with_a_different_version)
{
    auto connection = connect();
    auto ext = mir_extension_favorite_flavor_v9(connection);
    ASSERT_THAT(ext, Ne(nullptr));
    ASSERT_THAT(ext->favorite_flavor, Ne(nullptr));
    EXPECT_THAT(ext->favorite_flavor(), StrEq("rhubarb"));
    mir_connection_release(connection);
}

TEST_F(ClientExtensions, can_load_different_extensions)
{
    auto connection = connect();
    auto ext = mir_extension_animal_names_v1(connection);
    ASSERT_THAT(ext, Ne(nullptr));
    ASSERT_THAT(ext->animal_name, Ne(nullptr));
    EXPECT_THAT(ext->animal_name(), Not(StrEq("mushroom")));
    mir_connection_release(connection);
}

TEST_F(ClientExtensions, gives_nullptr_on_errors)
{
    auto connection = connect();
    int made_up_version = 10101;
    EXPECT_THAT(mir_connection_request_extension(
        connection, "mir_extension_animal_names", made_up_version), Eq(nullptr));
    EXPECT_THAT(mir_connection_request_extension(
        connection, "pancake", 8), Eq(nullptr));
    mir_connection_release(connection);
}

TEST_F(ClientExtensions, queries_for_extensions)
{
    auto connection = connect();
    auto ext = mir_extension_favorite_flavor_v1(connection);
    EXPECT_THAT(ext, Ne(nullptr));
    mir_connection_release(connection);

    mtf::disable_flavors();

    connection = connect();
    mir_connection_release(connection);
    EXPECT_THAT(ext, Eq(nullptr));
}
