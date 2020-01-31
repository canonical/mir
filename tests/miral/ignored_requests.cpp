/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "test_window_manager_tools.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;
using mir::operator<<;

namespace
{
X const display_left{0};
Y const display_top{0};
Width const display_width{640};
Height const display_height{480};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct IgnoredRequests;

struct IgnoredRequestParam
{
    std::string name;
    std::function<Window(IgnoredRequests&)> window;
};

std::ostream& operator<<(std::ostream& ostream, IgnoredRequestParam const& param)
{
    return ostream << param.name;
}

struct IgnoredRequests : mt::TestWindowManagerTools, WithParamInterface<IgnoredRequestParam>
{
    Size const window_size{200, 200};
    mir::shell::SurfaceSpecification simple_modification;

    void SetUp() override
    {
        simple_modification.state = mir_window_state_maximized;
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    auto create_window() -> Window
    {
        Window window;

        mir::scene::SurfaceCreationParameters creation_parameters;
        creation_parameters.type = mir_window_type_normal;
        creation_parameters.size = window_size;

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [&window](WindowInfo const& window_info)
                        { window = window_info.window(); }));

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(window);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return window;
    }

    auto create_destroyed_window() -> Window
    {
        auto const window = create_window();
        basic_window_manager.remove_surface(session, window);
        return window;
    }

    auto create_never_before_seen_window() -> Window
    {
        mir::scene::SurfaceCreationParameters creation_parameters;
        creation_parameters.type = mir_window_type_normal;
        creation_parameters.size = window_size;

        return Window{session, create_surface(session, creation_parameters)};
    }

    auto create_null_window() const -> Window
    {
        return Window{session, nullptr};
    }
};
}

TEST_P(IgnoredRequests, modify_unknown_surface_noops)
{
    auto const window = GetParam().window(*this);
    basic_window_manager.modify_surface(session, window, simple_modification);
}

TEST_P(IgnoredRequests, remove_unknown_surface_noops)
{
    auto const window = GetParam().window(*this);
    basic_window_manager.remove_surface(session, window);
}

TEST_P(IgnoredRequests, set_attribute_on_unknown_surface_noops)
{
    auto const window = GetParam().window(*this);
    basic_window_manager.set_surface_attribute(session, window, mir_window_attrib_state, mir_window_state_maximized);
}

TEST_P(IgnoredRequests, raise_unknown_surface_noops)
{
    auto const window = GetParam().window(*this);
    basic_window_manager.handle_raise_surface(session, window, 100);
}

TEST_P(IgnoredRequests, drag_and_drop_unknown_surface_noops)
{
    auto const window = GetParam().window(*this);
    basic_window_manager.handle_request_drag_and_drop(session, window, 100);
}

TEST_P(IgnoredRequests, move_unknown_surface_noops)
{
    auto const window = GetParam().window(*this);
    basic_window_manager.handle_request_move(session, window, 100);
}

TEST_P(IgnoredRequests, resize_unknown_surface_noops)
{
    auto const window = GetParam().window(*this);

    basic_window_manager.handle_request_resize(session, window, 100, mir_resize_edge_southeast);
}

TEST_F(IgnoredRequests, remove_session_twice_noops)
{
    basic_window_manager.remove_session(session);
    basic_window_manager.remove_session(session);
}

INSTANTIATE_TEST_SUITE_P(UnknownWindow, IgnoredRequests, ::testing::Values(
    IgnoredRequestParam{
        "null window",
        [](IgnoredRequests& test){ return test.create_null_window(); }},
    IgnoredRequestParam{
        "destroyed window",
        [](IgnoredRequests& test){ return test.create_destroyed_window(); }},
    IgnoredRequestParam{
        "never before seen window",
        [](IgnoredRequests& test){ return test.create_never_before_seen_window(); }}));

INSTANTIATE_TEST_SUITE_P(UnknownSession, IgnoredRequests, ::testing::Values(
    IgnoredRequestParam{
        "destroyed session",
        [](IgnoredRequests& test)
        {
            Window const window = test.create_window();
            test.basic_window_manager.remove_session(test.session);
            return window;
        }}));
