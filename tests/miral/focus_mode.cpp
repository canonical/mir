/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "test_window_manager_tools.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;

namespace
{
X const display_left{0};
Y const display_top{0};
Width const display_width{1280};
Height const display_height{720};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct FocusMode : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    auto create_window(mir::scene::SurfaceCreationParameters creation_parameters) -> Window
    {
        Window result;

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [&result](WindowInfo const& window_info)
                        { result = window_info.window(); }));

        creation_parameters.type = mir_window_type_normal;
        creation_parameters.size = Size{600, 400};
        basic_window_manager.add_surface(session, creation_parameters, &create_surface);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return result;
    }
};
}

TEST_F(FocusMode, new_window_has_focus_mode_created_with)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "disabled";
    params.focus_mode = mir_focus_mode_disabled;
    auto disabled = create_window(params);
    EXPECT_THAT(window_manager_tools.info_for(disabled).focus_mode(), Eq(mir_focus_mode_disabled));

    params.name = "focusable";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable = create_window(params);
    EXPECT_THAT(window_manager_tools.info_for(focusable).focus_mode(), Eq(mir_focus_mode_focusable));

    params.name = "grabbing";
    params.focus_mode = mir_focus_mode_grabbing;
    auto grabbing = create_window(params);
    EXPECT_THAT(window_manager_tools.info_for(grabbing).focus_mode(), Eq(mir_focus_mode_grabbing));

}

TEST_F(FocusMode, new_focusable_surface_becomes_active_by_default)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "focusable A";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable_a = create_window(params);

    params.name = "focusable B";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable_b = create_window(params);

    auto active = window_manager_tools.active_window();

    EXPECT_THAT(active, Eq(focusable_b));
}

TEST_F(FocusMode, new_grabbing_surface_becomes_active_by_default)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "focusable";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable = create_window(params);

    params.name = "grabbing";
    params.focus_mode = mir_focus_mode_grabbing;
    auto grabbing = create_window(params);

    auto active = window_manager_tools.active_window();

    EXPECT_THAT(active, Eq(grabbing));
}

TEST_F(FocusMode, new_disabled_surface_does_not_become_active_by_default)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "focusable";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable = create_window(params);

    params.name = "disabled";
    params.focus_mode = mir_focus_mode_disabled;
    auto disabled = create_window(params);

    auto active = window_manager_tools.active_window();

    EXPECT_THAT(active, Eq(focusable));
}

TEST_F(FocusMode, grabbing_surface_can_not_become_inactive)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "grabbing";
    params.focus_mode = mir_focus_mode_grabbing;
    auto grabbing = create_window(params);

    params.name = "focusable";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable = create_window(params);

    auto active = window_manager_tools.select_active_window(focusable);
    EXPECT_THAT(active, Eq(grabbing));
}

TEST_F(FocusMode, disabled_surface_can_not_become_active)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "focusable";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable = create_window(params);

    params.name = "disabled";
    params.focus_mode = mir_focus_mode_disabled;
    auto disabled = create_window(params);

    auto active = window_manager_tools.select_active_window(disabled);
    EXPECT_THAT(active, Eq(focusable));
}

TEST_F(FocusMode, grabbing_surface_can_not_steal_from_other_grabbing_surface)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "grabbing A";
    params.focus_mode = mir_focus_mode_grabbing;
    auto grabbing_a = create_window(params);

    params.name = "grabbing B";
    params.focus_mode = mir_focus_mode_grabbing;
    auto grabbing_b = create_window(params);

    auto active = window_manager_tools.select_active_window(grabbing_b);
    EXPECT_THAT(active, Eq(grabbing_a));
}

TEST_F(FocusMode, child_of_disabled_is_active)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "focusable";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable = create_window(params);

    params.name = "disabled";
    params.focus_mode = mir_focus_mode_disabled;
    auto disabled = create_window(params);

    params.name = "child-of-disabled";
    params.focus_mode = mir_focus_mode_focusable;
    params.parent = disabled;
    auto child_of_disabled = create_window(params);

    auto active = window_manager_tools.active_window();
    EXPECT_THAT(active, Eq(child_of_disabled));
}

TEST_F(FocusMode, disabled_does_not_gain_focus_when_child_closes)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "focusable";
    params.focus_mode = mir_focus_mode_focusable;
    auto focusable = create_window(params);

    params.name = "disabled";
    params.focus_mode = mir_focus_mode_disabled;
    auto disabled = create_window(params);

    params.name = "child-of-disabled";
    params.focus_mode = mir_focus_mode_focusable;
    params.parent = disabled;
    auto child_of_disabled = create_window(params);
    ASSERT_THAT(window_manager_tools.select_active_window(child_of_disabled), Eq(child_of_disabled));

    basic_window_manager.remove_surface(session, child_of_disabled);

    auto active = window_manager_tools.active_window();
    EXPECT_THAT(active, Eq(focusable));
}

TEST_F(FocusMode, gains_focus_on_disabled_to_grabbing)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "secondary";
    params.focus_mode = mir_focus_mode_focusable;
    auto secondary = create_window(params);

    params.name = "primary";
    params.focus_mode = mir_focus_mode_disabled;
    auto primary = create_window(params);

    miral::WindowSpecification spec;
    spec.focus_mode() = mir_focus_mode_grabbing;
    window_manager_tools.modify_window(window_manager_tools.info_for(primary), spec);

    auto active = window_manager_tools.active_window();
    EXPECT_THAT(active, Eq(primary));
}

TEST_F(FocusMode, gains_focus_on_unfocused_focusable_to_grabbing)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "secondary";
    params.focus_mode = mir_focus_mode_focusable;
    auto secondary = create_window(params);

    params.name = "primary";
    params.focus_mode = mir_focus_mode_focusable;
    auto primary = create_window(params);

    ASSERT_THAT(window_manager_tools.select_active_window(secondary), Eq(secondary));

    miral::WindowSpecification spec;
    spec.focus_mode() = mir_focus_mode_grabbing;
    window_manager_tools.modify_window(window_manager_tools.info_for(primary), spec);

    auto active = window_manager_tools.active_window();
    EXPECT_THAT(active, Eq(primary));
}

TEST_F(FocusMode, can_gain_focus_after_disabeled_to_focusable)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "secondary";
    params.focus_mode = mir_focus_mode_focusable;
    auto secondary = create_window(params);

    params.name = "primary";
    params.focus_mode = mir_focus_mode_disabled;
    auto primary = create_window(params);

    miral::WindowSpecification spec;
    spec.focus_mode() = mir_focus_mode_focusable;
    window_manager_tools.modify_window(window_manager_tools.info_for(primary), spec);

    auto active = window_manager_tools.select_active_window(primary);
    EXPECT_THAT(active, Eq(primary));
}

TEST_F(FocusMode, loses_focus_on_grabbing_to_disabeled)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "secondary";
    params.focus_mode = mir_focus_mode_focusable;
    auto secondary = create_window(params);

    params.name = "primary";
    params.focus_mode = mir_focus_mode_grabbing;
    auto primary = create_window(params);

    miral::WindowSpecification spec;
    spec.focus_mode() = mir_focus_mode_disabled;
    window_manager_tools.modify_window(window_manager_tools.info_for(primary), spec);

    auto active = window_manager_tools.active_window();
    EXPECT_THAT(active, Ne(primary));
}

TEST_F(FocusMode, loses_focus_on_focusable_to_disabeled)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "secondary";
    params.focus_mode = mir_focus_mode_focusable;
    auto secondary = create_window(params);

    params.name = "primary";
    params.focus_mode = mir_focus_mode_focusable;
    auto primary = create_window(params);

    ASSERT_THAT(window_manager_tools.select_active_window(primary), Eq(primary));

    miral::WindowSpecification spec;
    spec.focus_mode() = mir_focus_mode_disabled;
    window_manager_tools.modify_window(window_manager_tools.info_for(primary), spec);

    auto active = window_manager_tools.active_window();
    EXPECT_THAT(active, Ne(primary));
}

TEST_F(FocusMode, does_not_lose_focus_on_grabbing_to_focusable)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "secondary";
    params.focus_mode = mir_focus_mode_focusable;
    auto secondary = create_window(params);

    params.name = "primary";
    params.focus_mode = mir_focus_mode_grabbing;
    auto primary = create_window(params);

    miral::WindowSpecification spec;
    spec.focus_mode() = mir_focus_mode_focusable;
    window_manager_tools.modify_window(window_manager_tools.info_for(primary), spec);

    auto active = window_manager_tools.active_window();
    EXPECT_THAT(active, Eq(primary));
}

TEST_F(FocusMode, can_lose_focus_after_grabing_focusable)
{
    mir::scene::SurfaceCreationParameters params;

    params.name = "secondary";
    params.focus_mode = mir_focus_mode_focusable;
    auto secondary = create_window(params);

    params.name = "primary";
    params.focus_mode = mir_focus_mode_grabbing;
    auto primary = create_window(params);

    miral::WindowSpecification spec;
    spec.focus_mode() = mir_focus_mode_focusable;
    window_manager_tools.modify_window(window_manager_tools.info_for(primary), spec);

    auto active = window_manager_tools.select_active_window(secondary);
    EXPECT_THAT(active, Eq(secondary));
}
