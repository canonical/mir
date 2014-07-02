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

#include "mir_toolkit/mir_client_library.h"

#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

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

TEST(MirClientTest, mir_surface_is_valid_handles_invalid_pointers)
{
    MirSurface* null_pointer = NULL;
    double stack_variable;
    MirSurface* not_a_mir_surface_on_the_stack = reinterpret_cast<MirSurface*>(&stack_variable);
    auto heap_variable = std::make_shared<int>();
    MirSurface* not_a_mir_surface_on_the_heap = reinterpret_cast<MirSurface*>(heap_variable.get());

    ASSERT_FALSE(mir_surface_is_valid(null_pointer));
    ASSERT_FALSE(mir_surface_is_valid(not_a_mir_surface_on_the_stack));
    ASSERT_FALSE(mir_surface_is_valid(not_a_mir_surface_on_the_heap));
}
