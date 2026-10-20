/*
 * Copyright © Canonical Ltd.
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
 */

#include "test_window_manager_tools.h"
#include <mir/event_printer.h>

#include <array>
#include <vector>

using namespace miral;
using namespace testing;
namespace mt = mir::test;
using mir::operator<<;

namespace
{
struct WindowTypeChange
{
    MirWindowType from;
    MirWindowType to;
    bool accepted;
};

auto window_type_changes() -> std::vector<WindowTypeChange>
{
    constexpr std::array sources{
        mir_window_type_normal,
        mir_window_type_utility,
        mir_window_type_dialog,
        mir_window_type_gloss,
        mir_window_type_freestyle,
        mir_window_type_menu,
        mir_window_type_inputmethod,
        mir_window_type_satellite,
        mir_window_type_tip};
    constexpr std::array targets{
        mir_window_type_normal,
        mir_window_type_utility,
        mir_window_type_dialog,
        mir_window_type_gloss,
        mir_window_type_freestyle,
        mir_window_type_menu,
        mir_window_type_inputmethod,
        mir_window_type_satellite,
        mir_window_type_tip,
        mir_window_type_decoration};

    std::vector<WindowTypeChange> changes;
    for (auto const from : sources)
    {
        for (auto const to : targets)
        {
            if (from == to)
                continue;

            auto const accepted =
                ((to == mir_window_type_normal ||
                  to == mir_window_type_utility ||
                  to == mir_window_type_satellite ||
                  to == mir_window_type_freestyle) &&
                 (from == mir_window_type_normal ||
                  from == mir_window_type_utility ||
                  from == mir_window_type_dialog ||
                  from == mir_window_type_satellite ||
                  from == mir_window_type_freestyle)) ||
                (to == mir_window_type_dialog &&
                 (from == mir_window_type_normal ||
                  from == mir_window_type_utility ||
                  from == mir_window_type_dialog ||
                  from == mir_window_type_menu ||
                  from == mir_window_type_satellite ||
                  from == mir_window_type_freestyle));
            changes.push_back({from, to, accepted});
        }
    }
    return changes;
}

X const display_left{0};
Y const display_top{0};
Width const display_width{640};
Height const display_height{480};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct ModifyWindowState : mt::TestWindowManagerTools, WithParamInterface<WindowTypeChange>
{
    Size const initial_parent_size{600, 400};

    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    auto create_window_of_type(MirWindowType type, Window const& parent = Window{}) -> Window
    {
        Window window;

        mir::shell::SurfaceSpecification creation_parameters;
        creation_parameters.type = type;
        creation_parameters.parent = parent;
        creation_parameters.set_size(initial_parent_size);

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                    [&window](WindowInfo const& window_info)
                        { window = window_info.window(); });

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(window);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return window;
    }
};
}

TEST_F(ModifyWindowState, can_divorce_parent)
{
    auto const parent = create_window_of_type(mir_window_type_normal);
    auto const child  = create_window_of_type(mir_window_type_freestyle, parent);

    mir::shell::SurfaceSpecification modifications;
    modifications.parent = Window{};

    basic_window_manager.modify_surface(session, child, modifications);
}

TEST_F(ModifyWindowState, freestyle_window_can_be_retyped)
{
    auto const window = create_window_of_type(mir_window_type_freestyle);

    mir::shell::SurfaceSpecification modifications;
    modifications.type = mir_window_type_utility;

    basic_window_manager.modify_surface(session, window, modifications);

    EXPECT_THAT(window_manager_tools.info_for(window).type(), Eq(mir_window_type_utility));
}

TEST_F(ModifyWindowState, window_can_be_retyped_back_to_freestyle)
{
    auto const window = create_window_of_type(mir_window_type_dialog);

    mir::shell::SurfaceSpecification modifications;
    modifications.type = mir_window_type_freestyle;

    basic_window_manager.modify_surface(session, window, modifications);

    EXPECT_THAT(window_manager_tools.info_for(window).type(), Eq(mir_window_type_freestyle));
}

TEST_F(ModifyWindowState, retyping_a_window_to_its_current_type_is_accepted)
{
    auto const window = create_window_of_type(mir_window_type_normal);

    mir::shell::SurfaceSpecification modifications;
    modifications.type = mir_window_type_normal;

    basic_window_manager.modify_surface(session, window, modifications);

    EXPECT_THAT(window_manager_tools.info_for(window).type(), Eq(mir_window_type_normal));
}

TEST_F(ModifyWindowState, unsupported_type_change_is_rejected)
{
    auto const window = create_window_of_type(mir_window_type_normal);

    mir::shell::SurfaceSpecification modifications;
    modifications.type = mir_window_type_inputmethod;

    EXPECT_THROW(basic_window_manager.modify_surface(session, window, modifications), std::runtime_error);
}

TEST_P(ModifyWindowState, type_change_is_accepted_only_when_supported)
{
    auto const parent = create_window_of_type(mir_window_type_normal);
    auto const [from, to, accepted] = GetParam();
    auto const window = create_window_of_type(
        from,
        from == mir_window_type_normal || from == mir_window_type_utility ? Window{} : parent);

    mir::shell::SurfaceSpecification modifications;
    modifications.type = to;
    if (to == mir_window_type_normal || to == mir_window_type_utility)
        modifications.parent = Window{};
    else if (to == mir_window_type_satellite)
        modifications.parent = parent;

    if (accepted)
    {
        basic_window_manager.modify_surface(session, window, modifications);
        EXPECT_THAT(window_manager_tools.info_for(window).type(), Eq(to));
    }
    else
    {
        EXPECT_THROW(basic_window_manager.modify_surface(session, window, modifications), std::runtime_error);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ModifyWindowType,
    ModifyWindowState,
    ValuesIn(window_type_changes()));
