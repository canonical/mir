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

#include "mir/input/cursor_observer_multiplexer.h"
#include "mir/shell/shell_wrapper.h"
#include "mir/shell/surface_stack_wrapper.h"

#include "mir/geometry/point.h"
#include "mir/main_loop.h"

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

struct MyCursorObserverMultiplexer : mi::CursorObserverMultiplexer
{
    MyCursorObserverMultiplexer(
        std::shared_ptr<mi::CursorObserverMultiplexer> const& wrapped, mir::Executor& default_executor) :
        mi::CursorObserverMultiplexer{default_executor},
        wrapped{wrapped}
    {
    }

    MOCK_METHOD2(cursor_moved_to, void(float abs_x, float abs_y));

    void pointer_usable() { wrapped->pointer_usable(); }
    void pointer_unusable() { wrapped->pointer_unusable(); }

    std::shared_ptr<mi::CursorObserverMultiplexer> const wrapped;
};

struct MySurfaceStack : msh::SurfaceStackWrapper
{
    using msh::SurfaceStackWrapper::SurfaceStackWrapper;
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

        server.wrap_cursor_observer_multiplexer([this]
            (std::shared_ptr<mi::CursorObserverMultiplexer> const& wrapped)
            {
                return std::make_shared<MyCursorObserverMultiplexer>(wrapped, *server.the_main_loop());
            });

        server.wrap_surface_stack([]
            (std::shared_ptr<msh::SurfaceStack> const& wrapped)
            {
                return std::make_shared<MySurfaceStack>(wrapped);
            });

        server.apply_settings();

        shell = server.the_shell();
        cursor_observer_multiplexer = server.the_cursor_observer_multiplexer();
        surface_stack = server.the_surface_stack();
    }

    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<mi::CursorObserverMultiplexer> cursor_observer_multiplexer;
    std::shared_ptr<msh::SurfaceStack> surface_stack;
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

TEST_F(ServerConfigurationWrapping, cursor_observer_multiplexer_is_of_wrapper_type)
{
    auto const my_cursor_observer_multiplexer = std::dynamic_pointer_cast<MyCursorObserverMultiplexer>(cursor_observer_multiplexer);

    EXPECT_THAT(my_cursor_observer_multiplexer, Ne(nullptr));
}

TEST_F(ServerConfigurationWrapping, can_override_cursor_observer_methods)
{
    float const abs_x{1};
    float const abs_y(2);

    auto const my_cursor_observer_multiplexer = std::dynamic_pointer_cast<MyCursorObserverMultiplexer>(cursor_observer_multiplexer);

    EXPECT_CALL(*my_cursor_observer_multiplexer, cursor_moved_to(abs_x, abs_y)).Times(1);
    cursor_observer_multiplexer->cursor_moved_to(abs_x, abs_y);
}

TEST_F(ServerConfigurationWrapping, returns_same_cursor_observer_multiplexer_from_cache)
{
    ASSERT_THAT(server.the_cursor_observer_multiplexer(), Eq(cursor_observer_multiplexer));
}

TEST_F(ServerConfigurationWrapping, surface_stack_is_of_wrapper_type)
{
    auto const my_surface_stack = std::dynamic_pointer_cast<MySurfaceStack>(surface_stack);

    EXPECT_THAT(my_surface_stack, Ne(nullptr));
}
