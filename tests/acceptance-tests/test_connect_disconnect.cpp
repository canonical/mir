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
 *              Thomas Guest <thomas.guest@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "display_server_test_fixture.h"

#include "mir_client/mir_surface.h"

#include <gtest/gtest.h>


TEST_F(DefaultDisplayServerTestFixture, client_connects_and_disconnects)
{
    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            using ::mir::client::Surface;

            // Default surface is not connected
            Surface mysurface;
            EXPECT_FALSE(mysurface.is_valid());

            // connect surface
            EXPECT_NO_THROW(mysurface = Surface(mir::test_socket_file(), 640, 480, 0));
            EXPECT_TRUE(mysurface.is_valid());

            // disconnect surface
            EXPECT_NO_THROW(mysurface = Surface());
            EXPECT_FALSE(mysurface.is_valid());
        }
    } client_connects_and_disconnects;

    launch_client_process(client_connects_and_disconnects);
}
