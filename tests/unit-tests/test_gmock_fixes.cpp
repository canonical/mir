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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/test/gmock_fixes.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>


TEST(GMock, return_by_move)
{
    struct Interface
    {
        virtual ~Interface() = default;
        virtual std::unique_ptr<int> function() const = 0;
    };

    struct MockImplementation : Interface
    {
        MOCK_CONST_METHOD0(function, std::unique_ptr<int>());
        ~MockImplementation() noexcept {}
    };

    MockImplementation mi;

    EXPECT_CALL(mi, function());

    mi.function();
}

TEST(GMock, return_by_move_with_deleter_function)
{
    struct Interface
    {
        virtual ~Interface() = default;
        virtual std::unique_ptr<int, void(*)(int*)> function() const = 0;
    };

    struct MockImplementation : Interface
    {
        MOCK_CONST_METHOD0(function, std::unique_ptr<int, void(*)(int*)>());
        ~MockImplementation() noexcept {}
    };

    MockImplementation mi;

    EXPECT_CALL(mi, function());

    mi.function();
}
