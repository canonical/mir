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
#include "mir_toolkit/mir_extension_core.h"

namespace mtf = mir_test_framework;
using namespace testing;

struct ClientExtensions : mtf::ConnectedClientHeadlessServer
{
};

TEST_F(ClientExtensions, can_load_an_extension)
{
    auto ext_raw = mir_connection_request_interface(
        connection, MIR_EXTENSION_FAVORITE_FLAVOR, MIR_EXTENSION_FAVORITE_FLAVOR_VERSION_1);
    ASSERT_THAT(ext_raw, Ne(nullptr));

    auto ext = static_cast<MirExtensionFavoriteFlavor*>(ext_raw);
    ASSERT_THAT(ext->favorite_flavor, Ne(nullptr));
    EXPECT_THAT(ext->favorite_flavor(), StrEq("banana"));
}

TEST_F(ClientExtensions, can_load_an_extension_with_a_different_version)
{
    auto ext_raw = mir_connection_request_interface(
        connection, MIR_EXTENSION_FAVORITE_FLAVOR, MIR_EXTENSION_FAVORITE_FLAVOR_VERSION_2);
    ASSERT_THAT(ext_raw, Ne(nullptr));

    auto ext = static_cast<MirExtensionFavoriteFlavor*>(ext_raw);
    ASSERT_THAT(ext->favorite_flavor, Ne(nullptr));
    EXPECT_THAT(ext->favorite_flavor(), StrEq("rhubarb"));
}

TEST_F(ClientExtensions, can_load_different_extensions)
{
    auto ext_raw = mir_connection_request_interface(
        connection, MIR_EXTENSION_ANIMAL_NAME, MIR_EXTENSION_ANIMAL_NAME_VERSION_9);
    ASSERT_THAT(ext_raw, Ne(nullptr));

    auto ext = static_cast<MirExtensionAnimalNames*>(ext_raw);
    ASSERT_THAT(ext->animal_name, Ne(nullptr));
    EXPECT_THAT(ext->animal_name(), Not(StrEq("mushroom")));
}

TEST_F(ClientExtensions, gives_nullptr_on_errors)
{
    EXPECT_THAT(mir_connection_request_interface(
        connection, MIR_EXTENSION_ANIMAL_NAME, MIR_EXTENSION_VERSION(1,0)), Eq(nullptr));
    EXPECT_THAT(mir_connection_request_interface(
        connection, "pancake", MIR_EXTENSION_VERSION(1,0)), Eq(nullptr));
}
