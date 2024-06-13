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
#include <mir/scene/surface_stack.h>
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

/// A harness for window management testing. To use, extend this class and provide the
/// necessary virtual methods. You may use WindowManagementVerifier to assert that
/// calls to "advise*" are called correctly in response to input into the system.
class WindowManagementTestHarness : public testing::Test
{
public:
    void SetUp() override;

    auto open_application(std::string const& name) -> miral::Application;
    auto create_window(
        miral::Application const&,
        mir::geometry::Width const&,
        mir::geometry::Height const&) -> miral::Window;
    void publish_event(MirEvent const& event);
    void request_resize(miral::Window const&, MirInputEvent const*, MirResizeEdge);

    auto focused_surface() -> std::shared_ptr<mir::scene::Surface>;
    [[nodiscard]] auto stacking_order_of(mir::scene::SurfaceSet const& surfaces) const -> mir::scene::SurfaceList;

    virtual auto get_builder() -> WindowManagementPolicyBuilder = 0;
    virtual auto get_output_rectangles() -> std::vector<mir::geometry::Rectangle> = 0;

    struct Self;
    std::shared_ptr<Self> self;
};

}

#endif //MIR_TEST_FRAMEWORK_WINDOW_MANAGEMENT_TEST_HARNESS_H
