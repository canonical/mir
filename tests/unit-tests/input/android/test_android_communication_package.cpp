/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/android/android_input_channel.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unistd.h>
#include <fcntl.h>

namespace droidinput = android;
namespace mia = mir::input::android;

TEST(AndroidInputChannel, packages_own_valid_fds)
{
    int server_fd, client_fd;
    {
        mia::AndroidInputChannel package;

        server_fd = package.server_fd();
        client_fd = package.client_fd();
        EXPECT_LE(0, server_fd);
        EXPECT_LE(0, client_fd);

        EXPECT_EQ(fcntl(server_fd, F_GETFD), 0);
        EXPECT_EQ(fcntl(client_fd, F_GETFD), 0);
    }
    EXPECT_LT(fcntl(server_fd, F_GETFD), 0);
    EXPECT_LT(fcntl(client_fd, F_GETFD), 0);
}
