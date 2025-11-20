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
#include <mir/test/doubles/stub_session.h>

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

    MOCK_METHOD(void, advise_new_window, (miral::WindowInfo const& window_info), (override));
    MOCK_METHOD(void, advise_move_to, (miral::WindowInfo const& window_info, mir::geometry::Point top_left), (override));
    MOCK_METHOD(void, advise_resize, (miral::WindowInfo const& window_info, mir::geometry::Size const& new_size), (override));
    MOCK_METHOD(void, advise_raise, (std::vector<miral::Window> const&), (override));
    MOCK_METHOD(void, advise_output_create, (miral::Output const&), (override));
    MOCK_METHOD(void, advise_output_update, (miral::Output const&, miral::Output const&), (override));
    MOCK_METHOD(void, advise_output_delete, (miral::Output const&), (override));
    MOCK_METHOD(void, advise_application_zone_create, (miral::Zone const&), (override));
    MOCK_METHOD(void, advise_application_zone_update, (miral::Zone const&, miral::Zone const&), (override));
    MOCK_METHOD(void, advise_application_zone_delete, (miral::Zone const&), (override));

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
    /// the active window.
    /// \param creation_parameters
    /// \returns The active window
    auto create_and_select_window(mir::shell::SurfaceSpecification& creation_parameters) -> miral::Window;

    /// Creates a new session, adds the provided surface to the session, and then sets the resulting window as
    /// the active window.
    auto create_and_select_window_for_session(mir::shell::SurfaceSpecification&, std::shared_ptr<scene::Session>) -> miral::Window;
};

struct StubStubSession : mir::test::doubles::StubSession
{
    auto create_surface(
        std::shared_ptr<mir::scene::Session> const & /*session*/,
        mir::wayland::Weak<mir::frontend::WlSurface> const & /*wayland_surface*/,
        mir::shell::SurfaceSpecification const &params,
        std::shared_ptr<mir::scene::SurfaceObserver> const & /*observer*/,
        mir::Executor *) -> std::shared_ptr<mir::scene::Surface> override;

private:
    std::atomic<int> next_surface_id;
    std::map<mir::frontend::SurfaceId, std::shared_ptr<mir::scene::Surface>> surfaces;
};
}

}

#endif //MIRAL_TEST_WINDOW_MANAGER_TOOLS_H
