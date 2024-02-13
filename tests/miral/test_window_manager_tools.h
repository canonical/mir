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

#ifndef MIRAL_TEST_WINDOW_MANAGER_TOOLS_H
#define MIRAL_TEST_WINDOW_MANAGER_TOOLS_H

#include "basic_window_manager.h"

#include <miral/canonical_window_manager.h>
#include <miral/window.h>
#include <mir/shell/surface_specification.h>
#include <mir/scene/surface.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace graphics
{
class DisplayConfiguration;
}

namespace test
{

struct MockWindowManagerPolicy
    : miral::CanonicalWindowManagerPolicy
{
    using miral::CanonicalWindowManagerPolicy::CanonicalWindowManagerPolicy;

    bool handle_touch_event(MirTouchEvent const* /*event*/) { return false; }
    bool handle_pointer_event(MirPointerEvent const* /*event*/) { return false; }
    bool handle_keyboard_event(MirKeyboardEvent const* /*event*/) { return false; }

    MOCK_METHOD1(advise_new_window, void (miral::WindowInfo const& window_info));
    MOCK_METHOD2(advise_move_to, void(miral::WindowInfo const& window_info, mir::geometry::Point top_left));
    MOCK_METHOD2(advise_resize, void(miral::WindowInfo const& window_info, mir::geometry::Size const& new_size));
    MOCK_METHOD1(advise_raise, void(std::vector<miral::Window> const&));
    MOCK_METHOD1(advise_output_create, void(miral::Output const&));
    MOCK_METHOD2(advise_output_update, void(miral::Output const&, miral::Output const&));
    MOCK_METHOD1(advise_output_delete, void(miral::Output const&));
    MOCK_METHOD1(advise_application_zone_create, void(miral::Zone const&));
    MOCK_METHOD2(advise_application_zone_update, void(miral::Zone const&, miral::Zone const&));
    MOCK_METHOD1(advise_application_zone_delete, void(miral::Zone const&));

    void handle_request_move(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/) {}
    void handle_request_resize(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/, MirResizeEdge /*edge*/) {}
    mir::geometry::Rectangle confirm_placement_on_display(const miral::WindowInfo&, MirWindowState, mir::geometry::Rectangle const& new_placement)
    {
        return new_placement;
    }
};

class TestWindowManagerTools : public testing::Test
{
private:
    struct Self;
    std::unique_ptr<Self> self;

public:
    TestWindowManagerTools();
    ~TestWindowManagerTools();

    std::shared_ptr<mir::scene::Session> session;
    MockWindowManagerPolicy* window_manager_policy;
    miral::WindowManagerTools window_manager_tools;
    miral::BasicWindowManager basic_window_manager;

    static auto create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::shell::SurfaceSpecification const& params) -> std::shared_ptr<mir::scene::Surface>;

    auto static create_fake_display_configuration(std::vector<miral::Rectangle> const& outputs)
        -> std::shared_ptr<graphics::DisplayConfiguration const>;
    auto static create_fake_display_configuration(
        std::vector<std::pair<graphics::DisplayConfigurationLogicalGroupId, miral::Rectangle>> const& outputs)
        -> std::shared_ptr<graphics::DisplayConfiguration const>;
    void notify_configuration_applied(
        std::shared_ptr<graphics::DisplayConfiguration const> display_config);

    /// Creates a new session, adds a surface to the session, and then sets the resulting window as
    /// the active window.z
    /// \param creation_parameters
    /// \returns The active window
    auto create_and_select_window(mir::shell::SurfaceSpecification creation_parameters) -> miral::Window;
};

}
}

#endif //MIRAL_TEST_WINDOW_MANAGER_TOOLS_H
