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

#include <miral/internal_client.h>
#include <examples/example-server-lib/floating_window_manager.h>
#include <examples/example-server-lib/splash_session.h>
#include <mir_test_framework/window_management_test_harness.h>

namespace geom = mir::geometry;

class StubSplashSession : public SplashSession
{
public:
    auto session() const -> std::shared_ptr<mir::scene::Session> override
    {
        return nullptr;
    }
};

class FloatingWindowManagerTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<FloatingWindowManagerPolicy>(
                tools,
                std::make_shared<StubSplashSession>(),
                launcher,
                shutdown_hook
            );
        };
    }

    auto get_output_rectangles() -> std::vector<mir::geometry::Rectangle> override
    {
        return {
            mir::geometry::Rectangle{{0, 0}, {800, 600}},
            mir::geometry::Rectangle{{800, 0}, {800, 600}}
        };
    }

private:
    miral::InternalClientLauncher launcher;
    std::function<void()> shutdown_hook = []{};
};

TEST_F(FloatingWindowManagerTest, can_switch_workspace)
{
    auto app = open_application("app");
    miral::WindowSpecification spec;
    spec.size() = geom::Size(100, 100);
    spec.top_left() = geom::Point(50, 50);
    auto window = create_window(app, spec);
    tools().for_each_workspace_containing(window, [](std::shared_ptr<miral::Workspace> const& workspace)
    {
        EXPECT_TRUE(workspace->)
    });
}
