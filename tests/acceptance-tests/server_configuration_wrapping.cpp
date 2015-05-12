/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/input/cursor_listener.h"
#include "mir/shell/shell_wrapper.h"

#include "mir_test_framework/headless_test.h"

#include "mir_test_framework/executable_path.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
struct MyShell : msh::ShellWrapper
{
    using msh::ShellWrapper::ShellWrapper;
    MOCK_METHOD0(focus_next_session, void());
};

struct MyCursorListener : mi::CursorListener
{
    MyCursorListener(std::shared_ptr<mi::CursorListener> const& wrapped) :
        wrapped{wrapped} {}

    MOCK_METHOD2(cursor_moved_to, void(float abs_x, float abs_y));

    std::shared_ptr<mi::CursorListener> const wrapped;
};

struct ServerConfigurationWrapping : mir_test_framework::HeadlessTest
{
    void SetUp() override
    {
        server.wrap_shell([]
            (std::shared_ptr<msh::Shell> const& wrapped)
            {
                return std::make_shared<MyShell>(wrapped);
            });

        server.wrap_cursor_listener([]
            (std::shared_ptr<mi::CursorListener> const& wrapped)
            {
                return std::make_shared<MyCursorListener>(wrapped);
            });

        server.apply_settings();

        shell = server.the_shell();
        cursor_listener = server.the_cursor_listener();
    }

    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<mi::CursorListener> cursor_listener;
};
}

TEST_F(ServerConfigurationWrapping, shell_is_of_wrapper_type)
{
    auto const my_shell = std::dynamic_pointer_cast<MyShell>(shell);

    EXPECT_THAT(my_shell, Ne(nullptr));
}

TEST_F(ServerConfigurationWrapping, can_override_shell_methods)
{
    auto const my_shell = std::dynamic_pointer_cast<MyShell>(shell);

    EXPECT_CALL(*my_shell, focus_next_session()).Times(1);
    shell->focus_next_session();
}

TEST_F(ServerConfigurationWrapping, returns_same_shell_from_cache)
{
    ASSERT_THAT(server.the_shell(), Eq(shell));
}

TEST_F(ServerConfigurationWrapping, cursor_listener_is_of_wrapper_type)
{
    auto const my_cursor_listener = std::dynamic_pointer_cast<MyCursorListener>(cursor_listener);

    EXPECT_THAT(my_cursor_listener, Ne(nullptr));
}

TEST_F(ServerConfigurationWrapping, can_override_cursor_listener_methods)
{
    float const abs_x{1};
    float const abs_y(2);

    auto const my_cursor_listener = std::dynamic_pointer_cast<MyCursorListener>(cursor_listener);

    EXPECT_CALL(*my_cursor_listener, cursor_moved_to(abs_x, abs_y)).Times(1);
    cursor_listener->cursor_moved_to(abs_x, abs_y);
}

TEST_F(ServerConfigurationWrapping, returns_same_cursor_listener_from_cache)
{
    ASSERT_THAT(server.the_cursor_listener(), Eq(cursor_listener));
}
