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

#ifndef MIR_TEST_FRAMEWORK_WINDOW_MANAGEMENT_TEST_HARNESS_H
#define MIR_TEST_FRAMEWORK_WINDOW_MANAGEMENT_TEST_HARNESS_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <miral/window_management_policy.h>
#include <miral/application.h>
#include "mir/test/doubles/fake_display_configuration_observer_registrar.h"

namespace mir
{
namespace scene
{
class SurfaceStack;
class Surface;
class Session;
}
namespace shell
{
class AbstractShell;
}
namespace compositor
{
class BufferStream;
}
}

namespace mir_test_framework
{
using WindowManagementPolicyBuilder =
    std::function<std::unique_ptr<miral::WindowManagementPolicy>(miral::WindowManagerTools const& tools)>;

class WindowManagementVerifier
{
public:
    virtual ~WindowManagementVerifier() = default;
    virtual void advise_new_app(miral::ApplicationInfo&) {}
    virtual void advise_delete_app(miral::ApplicationInfo const&) {}
    virtual void advise_new_window(miral::WindowInfo const&) {}
    virtual void advise_focus_lost(miral::WindowInfo const&) {}
    virtual void advise_focus_gained(miral::WindowInfo const&) {}
    virtual void advise_state_change(miral::WindowInfo const&, MirWindowState) {}
    virtual void advise_move_to(miral::WindowInfo const&, mir::geometry::Point) {}
    virtual void advise_resize(miral::WindowInfo const&, mir::geometry::Size const&) {}
    virtual void advise_delete_window(miral::WindowInfo const&) {}
    virtual void advise_raise(std::vector<miral::Window> const&) {}
    virtual void advise_adding_to_workspace(
    std::shared_ptr<miral::Workspace> const&,
    std::vector<miral::Window> const&) {}
    virtual void advise_removing_from_workspace(
    std::shared_ptr<miral::Workspace> const&,
    std::vector<miral::Window> const&) {}
    virtual void advise_output_create(miral::Output const&) {}
    virtual void advise_output_update(miral::Output const&, miral::Output const&) {}
    virtual void advise_output_delete(miral::Output const&) {}
    virtual void advise_application_zone_create(miral::Zone const&) {}
    virtual void advise_application_zone_update(miral::Zone const&, miral::Zone const&) {}
    virtual void advise_application_zone_delete(miral::Zone const&) {}
};

/// A harness for window management testing. To use, extend this class and provide the
/// necessary virtual methods. You may use WindowManagementVerifier to assert that
/// calls to "advise*" are called correctly in response to input into the system.
class WindowManagementTestHarness : public testing::Test
{
public:
    void SetUp() override;

    miral::Application open_application(std::string const& name);
    miral::Window create_window(
        miral::Application const&,
        mir::geometry::Width const&,
        mir::geometry::Height const&);
    void publish_event(MirEvent const& event);
    void request_resize(miral::Window const&, MirInputEvent const*, MirResizeEdge);

    /// Returns the underlying abstract shell that the harness interacts with.
    /// WARNING: It is not advised to send commands directly to the shell, as
    /// that circumvents the harness. Please only use this to confirm state
    /// about the shell (e.g. the focused surface).
    [[nodiscard]] std::shared_ptr<mir::shell::AbstractShell> const& get_shell() const;

    /// Returns the underlying surface stack that the harness interacts with.
    /// WARNING: It is not advised to send commands directly to the surface stack, as
    /// that circumvents the harness. Please only use this to confirm state
    /// about the stack (e.g. the focus order).
    [[nodiscard]] std::shared_ptr<mir::scene::SurfaceStack> const& get_surface_stack() const;


    virtual std::shared_ptr<WindowManagementVerifier> get_verifier() = 0;
    virtual WindowManagementPolicyBuilder get_builder() = 0;
    virtual std::vector<mir::geometry::Rectangle> get_output_rectangles() = 0;

    struct Self;
    std::shared_ptr<Self> self;
};

}

#endif //MIR_TEST_FRAMEWORK_WINDOW_MANAGEMENT_TEST_HARNESS_H
