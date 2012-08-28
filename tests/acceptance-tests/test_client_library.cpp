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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_client/mir_client_library.h"

#include <gtest/gtest.h>

class TestConnection
{
public:
    TestConnection()
        : connected_(false)
    {
    }

    void connected(MirConnection * connection)
    {
        EXPECT_TRUE(mir_connection_is_valid(connection));
        EXPECT_STREQ(mir_connection_get_error_message(connection), "");
        connected_ = mir_connection_is_valid(connection);
    }

    bool is_connected() const
    {
        return connected_;
    }
private:
    bool connected_;
};

void connected_callback(MirConnection * connection, void * context)
{
    TestConnection * tc = reinterpret_cast<TestConnection *>(context);
    tc->connected(connection);
}

TEST(MirClient, connect)
{
    TestConnection tc;
    mir_connect(connected_callback, &tc);
    EXPECT_TRUE(tc.is_connected());
}
