/*
 * Copyright © 2021 Canonical Ltd.
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
#include <mir/event_printer.h>

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

struct FullscreenShellChrome : mt::TestWindowManagerTools, WithParamInterface<MirWindowType>
{
    Rectangle const panel_rect{{0, 0}, {display_width, 20}};
    Size const initial_size{600, 400};

    Window window;

    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    Window create_panel()
    {
        Window panel;

        mir::shell::SurfaceSpecification spec;
        spec.state = mir_window_state_attached;
        spec.set_size(panel_rect.size);
        spec.attached_edges = mir_placement_gravity_south;
        spec.exclusive_rect = panel_rect;

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce([&](WindowInfo const& window_info){ panel = window_info.window(); });

        basic_window_manager.add_surface(session, spec, &create_surface);
        basic_window_manager.select_active_window(panel);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return panel;
    }

    void create_window_of_type(MirWindowType type, MirWindowState state = mir_window_state_restored)
    {
        mir::shell::SurfaceSpecification spec;
        spec.type = type;
        spec.state = state;
        spec.set_size(initial_size);

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [this](WindowInfo const& window_info)
                        { window = window_info.window(); }));

        basic_window_manager.add_surface(session, spec, &create_surface);
        basic_window_manager.select_active_window(window);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);
    }

    void change_state_to(WindowInfo const& window_info, MirWindowState new_state)
    {
        WindowSpecification mods;
        mods.state() = new_state;
        window_manager_tools.place_and_size_for_state(mods, window_info);
        window_manager_tools.modify_window(window, mods);
    }
};

using None_OnFullscreen = FullscreenShellChrome;
using None_OnCreate = FullscreenShellChrome;
using None_OnAddPanel = FullscreenShellChrome;
using All_OnFullscreen = FullscreenShellChrome;
using All_OnCreate = FullscreenShellChrome;
using All_OnAddPanel = FullscreenShellChrome;

TEST_P(None_OnFullscreen, type)
{
    create_window_of_type(GetParam());
    auto const& info = window_manager_tools.info_for(window);

    change_state_to(info, mir_window_state_fullscreen);
    EXPECT_THAT(info.window().size(), Eq(display_area.size));

    change_state_to(info, mir_window_state_restored);
    EXPECT_THAT(info.window().size(), Eq(initial_size));
}

TEST_P(None_OnCreate, type)
{
    Window panel = create_panel();
    auto const app_zone = window_manager_tools.active_application_zone();
    ASSERT_THAT(app_zone.extents(), Ne(display_area));

    create_window_of_type(GetParam(), mir_window_state_fullscreen);
    auto const& info = window_manager_tools.info_for(window);

    EXPECT_THAT(info.window().size(), Eq(display_area.size));
}

TEST_P(None_OnAddPanel, type)
{
    create_window_of_type(GetParam(), mir_window_state_fullscreen);
    auto const& info = window_manager_tools.info_for(window);
    EXPECT_THAT(info.window().size(), Eq(display_area.size));

    Window panel = create_panel();
    auto const app_zone = window_manager_tools.active_application_zone();
    ASSERT_THAT(app_zone.extents(), Ne(display_area));

    EXPECT_THAT(info.window().size(), Eq(display_area.size));
}

TEST_P(All_OnFullscreen, type)
{
    Window panel = create_panel();
    auto const app_zone = window_manager_tools.active_application_zone();
    ASSERT_THAT(app_zone.extents(), Ne(display_area));

    window_manager_tools.set_fullscreen_shell_chrome(mir_fullscreen_shell_chrome_all);

    create_window_of_type(GetParam());
    auto const& info = window_manager_tools.info_for(window);

    change_state_to(info, mir_window_state_fullscreen);
    EXPECT_THAT(info.window().size(), Eq(app_zone.extents().size));

    change_state_to(info, mir_window_state_restored);
    EXPECT_THAT(info.window().size(), Eq(initial_size));
}

TEST_P(All_OnCreate, type)
{
    Window panel = create_panel();
    auto const app_zone = window_manager_tools.active_application_zone();
    ASSERT_THAT(app_zone.extents(), Ne(display_area));

    window_manager_tools.set_fullscreen_shell_chrome(mir_fullscreen_shell_chrome_all);

    create_window_of_type(GetParam(), mir_window_state_fullscreen);
    auto const& info = window_manager_tools.info_for(window);

    EXPECT_THAT(info.window().size(), Eq(app_zone.extents().size));
}

TEST_P(All_OnAddPanel, type)
{
    window_manager_tools.set_fullscreen_shell_chrome(mir_fullscreen_shell_chrome_all);

    create_window_of_type(GetParam(), mir_window_state_fullscreen);
    auto const& info = window_manager_tools.info_for(window);
    EXPECT_THAT(info.window().size(), Eq(display_area.size));

    Window panel = create_panel();
    auto const app_zone = window_manager_tools.active_application_zone();
    ASSERT_THAT(app_zone.extents(), Ne(display_area));

    EXPECT_THAT(info.window().size(), Eq(app_zone.extents().size));
}

INSTANTIATE_TEST_SUITE_P(FullscreenShellChrome, None_OnFullscreen, ::testing::Values(
    mir_window_type_normal,       /**< AKA "regular"                       */
    mir_window_type_utility,      /**< AKA "floating"                      */
    mir_window_type_dialog,
    mir_window_type_gloss,
    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,  /**< AKA "OSK" or handwriting etc.       */
    mir_window_type_satellite,    /**< AKA "toolbox"/"toolbar"             */
    mir_window_type_tip,          /**< AKA "tooltip"                       */
    mir_window_type_decoration
));

INSTANTIATE_TEST_SUITE_P(FullscreenShellChrome, None_OnCreate, ::testing::Values(
    mir_window_type_normal,       /**< AKA "regular"                       */
    mir_window_type_utility,      /**< AKA "floating"                      */
    mir_window_type_dialog,
    mir_window_type_gloss,
    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,  /**< AKA "OSK" or handwriting etc.       */
    mir_window_type_satellite,    /**< AKA "toolbox"/"toolbar"             */
    mir_window_type_tip,          /**< AKA "tooltip"                       */
    mir_window_type_decoration
));

INSTANTIATE_TEST_SUITE_P(FullscreenShellChrome, None_OnAddPanel, ::testing::Values(
    mir_window_type_normal,       /**< AKA "regular"                       */
    mir_window_type_utility,      /**< AKA "floating"                      */
    mir_window_type_dialog,
    mir_window_type_gloss,
    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,  /**< AKA "OSK" or handwriting etc.       */
    mir_window_type_satellite,    /**< AKA "toolbox"/"toolbar"             */
    mir_window_type_tip,          /**< AKA "tooltip"                       */
    mir_window_type_decoration
));

INSTANTIATE_TEST_SUITE_P(FullscreenShellChrome, All_OnFullscreen, ::testing::Values(
    mir_window_type_normal,       /**< AKA "regular"                       */
    mir_window_type_utility,      /**< AKA "floating"                      */
    mir_window_type_dialog,
    mir_window_type_gloss,
    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,  /**< AKA "OSK" or handwriting etc.       */
    mir_window_type_satellite,    /**< AKA "toolbox"/"toolbar"             */
    mir_window_type_tip,          /**< AKA "tooltip"                       */
    mir_window_type_decoration
));

INSTANTIATE_TEST_SUITE_P(FullscreenShellChrome, All_OnCreate, ::testing::Values(
    mir_window_type_normal,       /**< AKA "regular"                       */
    mir_window_type_utility,      /**< AKA "floating"                      */
    mir_window_type_dialog,
    mir_window_type_gloss,
    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,  /**< AKA "OSK" or handwriting etc.       */
    mir_window_type_satellite,    /**< AKA "toolbox"/"toolbar"             */
    mir_window_type_tip,          /**< AKA "tooltip"                       */
    mir_window_type_decoration
));

INSTANTIATE_TEST_SUITE_P(FullscreenShellChrome, All_OnAddPanel, ::testing::Values(
    mir_window_type_normal,       /**< AKA "regular"                       */
    mir_window_type_utility,      /**< AKA "floating"                      */
    mir_window_type_dialog,
    mir_window_type_gloss,
    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,  /**< AKA "OSK" or handwriting etc.       */
    mir_window_type_satellite,    /**< AKA "toolbox"/"toolbar"             */
    mir_window_type_tip,          /**< AKA "tooltip"                       */
    mir_window_type_decoration
));
}
