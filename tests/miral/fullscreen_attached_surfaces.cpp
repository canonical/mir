/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir_toolkit/common.h"
#include "test_window_manager_tools.h"

#include <mir/geometry/forward.h>
#include <mir/scene/surface.h>

#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace miral;
using namespace testing;
namespace mt = mir::test;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace ms = mir::scene;

namespace
{
geom::Rectangle const primary_output{{0, 0}, {1280, 720}};
geom::Rectangle const secondary_output{{1280, 0}, {960, 720}};
geom::Height const bar_height{32};

auto make_bar_spec(
    geom::Rectangle const& output_rect, std::optional<mg::DisplayConfigurationOutputId> output_id = std::nullopt)
    -> mir::shell::SurfaceSpecification
{
    mir::shell::SurfaceSpecification params;
    params.state = mir_window_state_attached;
    params.attached_edges = static_cast<MirPlacementGravity>(
        mir_placement_gravity_north | mir_placement_gravity_east | mir_placement_gravity_west);
    params.top_left = output_rect.top_left;
    params.set_size({output_rect.size.width, bar_height});
    params.exclusive_rect = geom::Rectangle{{0, 0}, {output_rect.size.width, bar_height}};
    params.depth_layer = mir_depth_layer_above;
    if (output_id)
        params.output_id = *output_id;
    return params;
}

auto make_app_spec(std::optional<mg::DisplayConfigurationOutputId> output_id = std::nullopt)
    -> mir::shell::SurfaceSpecification
{
    mir::shell::SurfaceSpecification params;
    params.state = mir_window_state_restored;
    params.width = Width{100};
    params.height = Height{100};
    params.depth_layer = mir_depth_layer_application;
    if (output_id)
        params.output_id = *output_id;
    return params;
}
}

struct FullscreenAttachedSurfaces : mt::TestWindowManagerTools
{
    void set_single_output()
    {
        notify_configuration_applied(create_fake_display_configuration({primary_output}));
    }

    void set_dual_outputs()
    {
        notify_configuration_applied(create_fake_display_configuration({primary_output, secondary_output}));
    }

    struct WindowMods
    {
        std::optional<MirWindowState> state = std::nullopt;
        std::optional<Point> top_left = std::nullopt;
    };

    auto create_window(mir::shell::SurfaceSpecification spec) -> std::pair<std::shared_ptr<mt::StubStubSession>, Window>
    {
        auto const session = std::make_shared<mt::StubStubSession>();
        auto const window = create_and_select_window_for_session(spec, session);
        return {session, window};
    }

    void modify_window(Window const& window, WindowMods const& mods)
    {
        WindowSpecification spec;
        if (mods.state)
            spec.state() = *mods.state;
        if (mods.top_left)
            spec.top_left() = *mods.top_left;
        window_manager_tools.modify_window(window, spec);
    }
};

TEST_F(FullscreenAttachedSurfaces, attached_above_hidden_while_fullscreen_and_restored_after_exit)
{
    set_single_output();

    auto const [bar_session, bar] = create_window(make_bar_spec(primary_output));
    auto const [app_session, app] = create_window(make_app_spec());

    auto const& bar_info = basic_window_manager.info_for(bar);
    EXPECT_THAT(bar_info.state(), Eq(mir_window_state_attached));

    modify_window(app, {mir_window_state_fullscreen});

    EXPECT_THAT(bar_info.state(), Eq(mir_window_state_hidden));

    modify_window(app, {mir_window_state_restored});

    EXPECT_THAT(bar_info.state(), Eq(mir_window_state_attached));
}

TEST_F(FullscreenAttachedSurfaces, only_attached_on_same_output_are_hidden)
{
    set_dual_outputs();

    auto const [bar1_session, bar1] = create_window(make_bar_spec(primary_output, mg::DisplayConfigurationOutputId{1}));
    auto const [bar2_session, bar2] = create_window(make_bar_spec(secondary_output, mg::DisplayConfigurationOutputId{2}));
    auto const [app_session, app] = create_window(make_app_spec(mg::DisplayConfigurationOutputId{1}));

    modify_window(app, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar1).state(), Eq(mir_window_state_hidden));
    EXPECT_THAT(basic_window_manager.info_for(bar2).state(), Eq(mir_window_state_attached));

    modify_window(app, {mir_window_state_restored});

    EXPECT_THAT(basic_window_manager.info_for(bar1).state(), Eq(mir_window_state_attached));

    modify_window(app, {.top_left = secondary_output.top_left});
    modify_window(app, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar1).state(), Eq(mir_window_state_attached));
    EXPECT_THAT(basic_window_manager.info_for(bar2).state(), Eq(mir_window_state_hidden));

    modify_window(app, {mir_window_state_restored});

    EXPECT_THAT(basic_window_manager.info_for(bar2).state(), Eq(mir_window_state_attached));
}

TEST_F(FullscreenAttachedSurfaces, focus_switch_restores_and_rehides_attached_surfaces)
{
    set_single_output();

    auto const [bar_session, bar] = create_window(make_bar_spec(primary_output));
    auto const [fullscreen_session, fullscreen] = create_window(make_app_spec());

    modify_window(fullscreen, {mir_window_state_fullscreen});
    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));

    auto const [normal_session, normal] = create_window(make_app_spec());

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_attached));

    basic_window_manager.select_active_window(fullscreen);

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));

    basic_window_manager.select_active_window(normal);

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_attached));
}

TEST_F(FullscreenAttachedSurfaces, closing_fullscreen_restores_attached_above_surfaces)
{
    set_single_output();

    auto const [bar_session, bar] = create_window(make_bar_spec(primary_output));
    auto const [app_session, app] = create_window(make_app_spec());

    modify_window(app, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));

    basic_window_manager.remove_surface(app_session, std::weak_ptr<ms::Surface>{std::shared_ptr<ms::Surface>(app)});

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_attached));
}

TEST_F(FullscreenAttachedSurfaces, removing_non_fullscreen_rehides_bar_when_fullscreen_remains)
{
    set_single_output();

    auto const [bar_session, bar] = create_window(make_bar_spec(primary_output));
    auto const [fullscreen_session, fullscreen] = create_window(make_app_spec());

    modify_window(fullscreen, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));

    auto const [normal_session, normal] = create_window(make_app_spec());

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_attached));

    basic_window_manager.remove_surface(
        normal_session, std::weak_ptr<ms::Surface>{std::shared_ptr<ms::Surface>(normal)});

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));
}

TEST_F(FullscreenAttachedSurfaces, child_windows_of_fullscreen_keep_attached_hidden)
{
    set_single_output();

    auto const [bar_session, bar] = create_window(make_bar_spec(primary_output));
    auto const [fullscreen_session, fullscreen] = create_window(make_app_spec());

    modify_window(fullscreen, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));

    mir::shell::SurfaceSpecification child_spec = make_app_spec();
    child_spec.parent = fullscreen;

    [[maybe_unused]] auto const child_pair = create_window(child_spec);

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));
}

TEST_F(FullscreenAttachedSurfaces, hidden_attached_surfaces_ignore_modify_surface)
{
    set_single_output();

    auto const [bar_session, bar] = create_window(make_bar_spec(primary_output));
    auto const [app_session, app] = create_window(make_app_spec());

    modify_window(app, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));

    mir::shell::SurfaceSpecification mods;
    mods.state = mir_window_state_attached;
    basic_window_manager.modify_surface(bar_session, std::shared_ptr<ms::Surface>(bar), mods);

    EXPECT_THAT(basic_window_manager.info_for(bar).state(), Eq(mir_window_state_hidden));
}

TEST_F(FullscreenAttachedSurfaces, switching_fullscreen_between_outputs_hides_attached_on_target_output)
{
    set_dual_outputs();

    auto const [bar1_session, bar1] = create_window(make_bar_spec(primary_output, mg::DisplayConfigurationOutputId{1}));
    auto const [bar2_session, bar2] = create_window(make_bar_spec(secondary_output, mg::DisplayConfigurationOutputId{2}));
    auto const [fullscreen1_session, fullscreen1] = create_window(make_app_spec(mg::DisplayConfigurationOutputId{1}));

    modify_window(fullscreen1, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar1).state(), Eq(mir_window_state_hidden));
    EXPECT_THAT(basic_window_manager.info_for(bar2).state(), Eq(mir_window_state_attached));

    auto const [fullscreen2_session, fullscreen2] = create_window(make_app_spec(mg::DisplayConfigurationOutputId{2}));

    modify_window(fullscreen2, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar1).state(), Eq(mir_window_state_hidden));
    EXPECT_THAT(basic_window_manager.info_for(bar2).state(), Eq(mir_window_state_hidden));
}

TEST_F(FullscreenAttachedSurfaces, removing_active_non_fullscreen_does_not_affect_other_output)
{
    set_dual_outputs();

    auto const [bar1_session, bar1] = create_window(make_bar_spec(primary_output, mg::DisplayConfigurationOutputId{1}));
    auto const [bar2_session, bar2] = create_window(make_bar_spec(secondary_output, mg::DisplayConfigurationOutputId{2}));
    auto const [fullscreen_session, fullscreen] = create_window(make_app_spec(mg::DisplayConfigurationOutputId{2}));

    modify_window(fullscreen, {mir_window_state_fullscreen});

    EXPECT_THAT(basic_window_manager.info_for(bar2).state(), Eq(mir_window_state_hidden));

    auto const [normal_session, normal] = create_window(make_app_spec(mg::DisplayConfigurationOutputId{1}));

    EXPECT_THAT(basic_window_manager.info_for(bar1).state(), Eq(mir_window_state_attached));

    basic_window_manager.remove_surface(normal_session, std::weak_ptr<ms::Surface>{std::shared_ptr<ms::Surface>(normal)});

    EXPECT_THAT(basic_window_manager.info_for(bar1).state(), Eq(mir_window_state_attached));
    EXPECT_THAT(basic_window_manager.info_for(bar2).state(), Eq(mir_window_state_hidden));
}
