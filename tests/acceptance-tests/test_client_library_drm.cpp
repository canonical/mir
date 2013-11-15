/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_drm.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

using MirClientLibraryDrmTest = DefaultDisplayServerTestFixture;

TEST_F(MirClientLibraryDrmTest, sets_gbm_device_in_platform_data)
{
    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            ASSERT_TRUE(connection);
            EXPECT_TRUE(mir_connection_is_valid(connection));

            struct gbm_device* dev = reinterpret_cast<struct gbm_device*>(connection);

            mir_connection_drm_set_gbm_device(connection, dev);

            MirPlatformPackage pkg;
            mir_connection_get_platform(connection, &pkg);
            EXPECT_EQ(sizeof(dev) / sizeof(int), static_cast<size_t>(pkg.data_items));
            EXPECT_EQ(dev, *reinterpret_cast<struct gbm_device**>(&pkg.data[0]));

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
