/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/client/error_buffer.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace geom = mir::geometry;
using namespace testing;

namespace
{
void buffer_callback(MirBuffer*, void* call_count_context)
{
    if (call_count_context)
        (*static_cast<int*>(call_count_context))++;
}

struct ErrorBuffer : Test
{
    ErrorBuffer()
    {
    }
    std::string error_message = "problemo";
    int rpc_id = -11;
};
}

TEST_F(ErrorBuffer, calls_callback_when_received)
{
    int call_count = 0;
    mcl::ErrorBuffer buffer(error_message, rpc_id, buffer_callback, &call_count);
    buffer.received();
    EXPECT_THAT(call_count, Eq(1));
}

TEST_F(ErrorBuffer, returns_correct_error_message)
{
    mcl::ErrorBuffer buffer(error_message, rpc_id, buffer_callback, nullptr);
    EXPECT_THAT(buffer.error_message(), StrEq(error_message));
}

TEST_F(ErrorBuffer, is_invalid)
{
    mcl::ErrorBuffer buffer(error_message, rpc_id, buffer_callback, nullptr);
    EXPECT_FALSE(buffer.valid());
}

TEST_F(ErrorBuffer, has_id)
{
    mcl::ErrorBuffer buffer(error_message, rpc_id, buffer_callback, nullptr);
    EXPECT_THAT(buffer.rpc_id(), Eq(rpc_id));
}

TEST_F(ErrorBuffer, throws_if_submitted)
{
    mcl::ErrorBuffer buffer(error_message, rpc_id, buffer_callback, nullptr);
    EXPECT_THROW({
        buffer.submitted();
    }, std::logic_error);
}
