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
#include <miral/window_management_policy.h>
#include <miral/application.h>
#include <miral/window_manager_tools.h>
#include <mir/events/event.h>
#include <mir/graphics/display_configuration.h>
#include "mir_test_framework/headless_in_process_server.h"

namespace mir::scene
{
class Surface;
class Session;
}

namespace mir_test_framework
{
using WindowManagementPolicyBuilder =
    std::function<std::unique_ptr<miral::WindowManagementPolicy>(miral::WindowManagerTools const& tools)>;

/// A harness for window management testing. To use, extend this class and provide the
/// necessary virtual methods.
class WindowManagementTestHarness : public HeadlessInProcessServer
{
public:
    WindowManagementTestHarness();
    ~WindowManagementTestHarness() override;

    void SetUp() override;
    void TearDown() override;

    auto open_application(std::string const& name) const -> miral::Application;

    /// Create a window with the provided spec
    auto create_window(
        miral::Application const&,
        miral::WindowSpecification const& window_spec) const -> miral::Window;

    void publish_event(MirEvent const& event) const;
    void request_resize(miral::Window const&, MirInputEvent const*, MirResizeEdge) const;
    void request_move(miral::Window const&, MirInputEvent const*) const;

    /// Simulates a wayland client requesting some modification from the compositor.
    void request_modify(miral::Window const&, miral::WindowSpecification const& spec);

    auto focused(miral::Window const&) const -> bool;
    auto tools() const -> miral::WindowManagerTools&;
    auto is_above(miral::Window const& a, miral::Window const& b) const -> bool;

    void for_each_output(std::function<void(miral::Output const&)> const& f) const;

    /// Simulates an update to the displays with the provided list of output configurations.
    void update_outputs(std::vector<mir::graphics::DisplayConfigurationOutput> const&) const;

    /// Helper method that transforms a list of output rectangles into a list of display configurations.
    /// This is useful for creating a list of simple, connected outptus quickly.
    static auto output_configs_from_output_rectangles(std::vector<mir::geometry::Rectangle> const& output_rects)
        -> std::vector<mir::graphics::DisplayConfigurationOutput>;

    virtual auto get_builder() -> WindowManagementPolicyBuilder = 0;
    virtual auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> = 0;
private:
    class Self;
    std::unique_ptr<Self> const self;
};

}

#endif //MIR_TEST_FRAMEWORK_WINDOW_MANAGEMENT_TEST_HARNESS_H
