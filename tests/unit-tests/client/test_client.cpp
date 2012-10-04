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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_client/mir_client_library.h"
#include "mir_buffer_package.h"

#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl=mir::client;

TEST(MirClientTest, mir_connection_is_valid_handles_invalid_pointers)
{
    using namespace testing;

    MirConnection *null_pointer = NULL;
    double stack_variable;
    MirConnection *not_a_mir_connection_on_the_stack = reinterpret_cast<MirConnection *>(&stack_variable);
    auto heap_variable = std::make_shared<int>();
    MirConnection *not_a_mir_connection_on_the_heap = reinterpret_cast<MirConnection *>(heap_variable.get());

    ASSERT_FALSE(mir_connection_is_valid(null_pointer));
    ASSERT_FALSE(mir_connection_is_valid(not_a_mir_connection_on_the_stack));
    ASSERT_FALSE(mir_connection_is_valid(not_a_mir_connection_on_the_heap));
}

/* todo: probably go away */
TEST(MirClientTest, mir_translates_buffer_ipc)
{
    MirBufferPackage client_package;
    client_package.data_items = 8;
    client_package.fd_items = 4;
    for (auto i=0; i < mir_buffer_package_max; i++)
    {
        if (i< client_package.data_items)
            client_package.data[i] = i * 3;
        else
            client_package.data[i] = -1;

        if (i< client_package.fd_items)
            client_package.fd[i] = i * 3 + 1;
        else
            client_package.data[i] = -1;
    }

    mcl::MirBufferPackage internal_package(client_package);


    ASSERT_EQ(client_package.data_items, (int) internal_package.data.size());
    ASSERT_EQ(client_package.fd_items, (int) internal_package.fd.size());

    int i=0;
    for(auto it = internal_package.data.begin(); it != internal_package.data.end(); it++)
    {
        EXPECT_EQ(*it, client_package.data[i++]);
    }
    i=0;
    for(auto it = internal_package.fd.begin(); it != internal_package.fd.end(); it++)
    {
        EXPECT_EQ(*it, client_package.fd[i++]);
    }
}
