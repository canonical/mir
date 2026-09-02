/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "protocol_error.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace mrs = mir::wayland_rs;

using namespace testing;

TEST(ProtocolError, accessors_return_constructor_values)
{
    mrs::ProtocolError const error{42, 7, "something went wrong"};

    EXPECT_THAT(error.object_id(), Eq(42u));
    EXPECT_THAT(error.code(), Eq(7u));
    EXPECT_THAT(error.message(), Eq("something went wrong"));
}

TEST(ProtocolError, message_excludes_sentinel_and_id_and_code)
{
    mrs::ProtocolError const error{42, 7, "just the message"};
    EXPECT_THAT(error.message(), Eq("just the message"));
}

TEST(ProtocolError, what_is_exact_encoded_wire_string)
{
    mrs::ProtocolError const error{42, 7, "something went wrong"};

    EXPECT_THAT(std::string{error.what()}, Eq("MIR_PROTOCOL_ERROR:42:7: something went wrong"));
}

TEST(ProtocolError, what_starts_with_sentinel)
{
    mrs::ProtocolError const error{1, 2, "boom"};

    EXPECT_THAT(std::string{error.what()}, StartsWith("MIR_PROTOCOL_ERROR:"));
}

TEST(ProtocolError, formats_printf_style_arguments)
{
    mrs::ProtocolError const error{3, 4, "bad value %d for %s", 99, "surface"};

    EXPECT_THAT(error.message(), Eq("bad value 99 for surface"));
    EXPECT_THAT(std::string{error.what()}, Eq("MIR_PROTOCOL_ERROR:3:4: bad value 99 for surface"));
}

TEST(ProtocolError, preserves_colons_in_message)
{
    mrs::ProtocolError const error{5, 6, "expected a:b:c format"};

    EXPECT_THAT(error.message(), Eq("expected a:b:c format"));
    EXPECT_THAT(std::string{error.what()}, Eq("MIR_PROTOCOL_ERROR:5:6: expected a:b:c format"));
}

TEST(ProtocolError, is_catchable_as_runtime_error)
{
    try
    {
        throw mrs::ProtocolError{8, 9, "thrown"};
    }
    catch (std::runtime_error const& error)
    {
        EXPECT_THAT(std::string{error.what()}, Eq("MIR_PROTOCOL_ERROR:8:9: thrown"));
        return;
    }

    FAIL() << "ProtocolError was not caught as std::runtime_error";
}
