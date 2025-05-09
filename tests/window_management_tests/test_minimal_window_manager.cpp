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

#define MIR_LOG_COMPONENT "test_minimal_window_manager"

#include "mir/geometry/forward.h"
#include "mir_test_framework/window_management_test_harness.h"
#include <miral/minimal_window_manager.h>
#include <miral/output.h>
#include <mir/scene/session.h>
#include <mir/wayland/weak.h>
#include <mir/scene/surface.h>
#include <mir/executor.h>
#include <mir/events/event_builders.h>
#include <mir/events/pointer_event.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/log.h>
#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/shell/shell.h>
#include <linux/input.h>

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mev = mir::events;
using namespace testing;

namespace
{
constexpr int exclusive_surface_size = 50;
typedef std::function<miral::WindowSpecification(geom::Size const& output_size)> CreateSurfaceSpecFunc;

}

class MinimalWindowManagerTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    MinimalWindowManagerTest() : WindowManagementTestHarness()
    {
        server.wrap_display_configuration_policy([&](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
            -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return std::make_shared<mir::graphics::SideBySideDisplayConfigurationPolicy>();
        });
    }

    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<miral::MinimalWindowManager>(tools, focus_stealing());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle{{0, 0}, {800, 600}},
            mir::geometry::Rectangle{{800, 0}, {1000, 600}}
        });
    }

    virtual miral::FocusStealing focus_stealing() const
    {
        return miral::FocusStealing::allow;
    }
};

TEST_F(MinimalWindowManagerTest, new_window_has_focus)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto window = create_window(app, spec);
    EXPECT_TRUE(focused(window));
}
