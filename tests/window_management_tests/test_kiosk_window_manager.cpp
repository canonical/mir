/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#define MIR_LOG_COMPONENT "test_kiosk_window_manager"

#include <mir_test_framework/window_management_test_harness.h>
#include <miral/kiosk_window_manager.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace geom = mir::geometry;
using namespace testing;

namespace
{
class KioskWindowManagerTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<miral::KioskWindowManagerPolicy>(tools);
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle{{0, 0}, {800, 600}},
        });
    }
};
}

TEST_F(KioskWindowManagerTest, new_normal_window_is_placed_fullscreen)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.size() = {geom::Width{100}, geom::Height{100}};
    auto const window = create_window(app, spec);

    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_fullscreen));
}

TEST_F(KioskWindowManagerTest, new_freestyle_window_is_placed_fullscreen)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_freestyle;
    spec.size() = {geom::Width{100}, geom::Height{100}};
    auto const window = create_window(app, spec);

    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_fullscreen));
}

TEST_F(KioskWindowManagerTest, new_normal_window_fills_the_output)
{
    auto const output = get_initial_output_configs()[0];
    auto const output_size = output.extents().size;

    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.size() = {geom::Width{100}, geom::Height{100}};
    auto const window = create_window(app, spec);

    EXPECT_THAT(tools().info_for(window).window().size(), Eq(output_size));
}

TEST_F(KioskWindowManagerTest, attached_window_is_not_forced_fullscreen)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.state() = mir_window_state_attached;
    spec.attached_edges() = mir_placement_gravity_south;
    spec.size() = {geom::Width{800}, geom::Height{200}};
    auto const window = create_window(app, spec);

    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_attached));
}

TEST_F(KioskWindowManagerTest, attached_window_retains_requested_size)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.state() = mir_window_state_attached;
    spec.attached_edges() = mir_placement_gravity_south;
    spec.size() = {geom::Width{800}, geom::Height{200}};
    auto const window = create_window(app, spec);

    EXPECT_THAT(tools().info_for(window).state(), Ne(mir_window_state_fullscreen));
}

TEST_F(KioskWindowManagerTest, child_window_is_not_forced_fullscreen)
{
    auto const app = open_application("test");

    miral::WindowSpecification parent_spec;
    parent_spec.type() = mir_window_type_normal;
    parent_spec.size() = {geom::Width{100}, geom::Height{100}};
    auto const parent = create_window(app, parent_spec);

    miral::WindowSpecification child_spec;
    child_spec.type() = mir_window_type_normal;
    child_spec.size() = {geom::Width{50}, geom::Height{50}};
    child_spec.parent() = parent;
    auto const child = create_window(app, child_spec);

    EXPECT_THAT(tools().info_for(child).state(), Ne(mir_window_state_fullscreen));
}

TEST_F(KioskWindowManagerTest, handle_modify_window_forces_kiosk_state_on_normal_window)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.size() = {geom::Width{100}, geom::Height{100}};
    auto const window = create_window(app, spec);

    ASSERT_THAT(tools().info_for(window).state(), Eq(mir_window_state_fullscreen));

    // Attempt to restore the window — kiosk should override it back to fullscreen
    miral::WindowSpecification modification;
    modification.state() = mir_window_state_restored;
    request_modify(window, modification);

    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_fullscreen));
}

TEST_F(KioskWindowManagerTest, handle_modify_window_does_not_force_kiosk_state_on_attached_window)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.state() = mir_window_state_attached;
    spec.attached_edges() = mir_placement_gravity_south;
    spec.size() = {geom::Width{800}, geom::Height{200}};
    auto const window = create_window(app, spec);

    ASSERT_THAT(tools().info_for(window).state(), Eq(mir_window_state_attached));

    // Modification that keeps the window attached — should not be kiosked
    miral::WindowSpecification modification;
    modification.size() = {geom::Width{800}, geom::Height{250}};
    request_modify(window, modification);

    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_attached));
}

namespace
{
class KioskWindowManagerMaximizedTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<miral::KioskWindowManagerPolicy>(tools, mir_window_state_maximized);
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle{{0, 0}, {800, 600}},
        });
    }
};
}

TEST_F(KioskWindowManagerMaximizedTest, new_normal_window_is_placed_maximized)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.size() = {geom::Width{100}, geom::Height{100}};
    auto const window = create_window(app, spec);

    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_maximized));
}

TEST_F(KioskWindowManagerMaximizedTest, attached_window_is_not_forced_maximized)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.type() = mir_window_type_normal;
    spec.state() = mir_window_state_attached;
    spec.attached_edges() = mir_placement_gravity_south;
    spec.size() = {geom::Width{800}, geom::Height{200}};
    auto const window = create_window(app, spec);

    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_attached));
}
