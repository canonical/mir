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
#include <miral/window_manager_tools.h>
#include <mir/shell/surface_specification.h>
#include <mir/events/event.h>
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
class WindowManagementTestHarness : public mir_test_framework::HeadlessInProcessServer
{
public:
    WindowManagementTestHarness();
    void SetUp() override;
    void TearDown() override;

    auto open_application(std::string const& name) -> miral::Application;

    /// Create a window with the provided spec
    auto create_window(
        miral::Application const&,
        mir::shell::SurfaceSpecification spec) -> miral::Window;

    void publish_event(MirEvent const& event);
    void request_resize(miral::Window const&, MirInputEvent const*, MirResizeEdge);
    void request_move(miral::Window const&, MirInputEvent const*);

    auto focused_surface() -> std::shared_ptr<mir::scene::Surface>;
    auto tools() -> miral::WindowManagerTools const&;
    virtual auto get_builder() -> WindowManagementPolicyBuilder = 0;
    virtual auto get_output_rectangles() -> std::vector<mir::geometry::Rectangle> = 0;

    struct Self;
    std::shared_ptr<Self> self;
};

}

#endif //MIR_TEST_FRAMEWORK_WINDOW_MANAGEMENT_TEST_HARNESS_H
