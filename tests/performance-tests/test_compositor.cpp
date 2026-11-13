/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "performance_test_utils.h"
#include "src/server/report/logging/compositor_report.h"

#include <mir/synchronised.h>
#include <mir/time/steady_clock.h>
#include <mir_test_framework/async_server_runner.h>
#include <mir_test_framework/executable_path.h>
#include <mir_test_framework/process.h>
#include <miral/external_client.h>
#include <miral/floating_window_manager.h>
#include <miral/set_window_management_policy.h>

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals::chrono_literals;
using namespace mir::test;
namespace mtf = mir_test_framework;
namespace mrl = mir::report::logging;

namespace
{
class CollectingCompositorReport : public mrl::CompositorReport
{
public:
    using mrl::CompositorReport::CompositorReport;

    auto latest_statistics() const -> std::optional<FrameStatistics>
    {
        return *statistics.lock();
    }

private:
    void report_statistics(SubCompositorId id, FrameStatistics const& sample) override
    {
        mrl::CompositorReport::report_statistics(id, sample);
        if (sample.frames > 0 && sample.elapsed > 0us)
            *statistics.lock() = sample;
    }

    mir::Synchronised<std::optional<FrameStatistics>> statistics;
};

struct CompositorPerformance : testing::Test, mtf::AsyncServerRunner
{
    void SetUp() override
    {
        window_management_policy(server);
        launcher(server);
        server.override_the_compositor_report(
            [this]
            {
                report = std::make_shared<CollectingCompositorReport>(
                    server.the_logger(), std::make_shared<mir::time::SteadyClock>());
                return report;
            });
        start_server();
    }

    void TearDown() override
    {
        clients.clear();
        stop_server();
        record_properties(output_filename, "server");
    }

    void spawn_clients(std::initializer_list<std::string> names)
    {
        for (auto const& name : names)
        {
            auto const pid = launcher.launch(std::vector<std::string>{mtf::executable_path() + "/" + name});
            ASSERT_GT(pid, 0);
            clients.push_back(std::make_unique<mtf::Process>(pid));
            std::this_thread::sleep_for(100ms);
        }
    }

    miral::SetWindowManagementPolicy const window_management_policy{
        miral::set_window_management_policy<miral::FloatingWindowManager>()};
    miral::ExternalClientLauncher launcher;
    std::shared_ptr<CollectingCompositorReport> report;
    std::vector<std::unique_ptr<mtf::Process>> clients;
};
} // anonymous namespace

TEST_F(CompositorPerformance, regression_test_1563287)
{
    spawn_clients({"mir_demo_client_wayland", "mir_demo_client_wayland_egl_spinner",
                   "mir_demo_client_wayland", "mir_demo_client_wayland_egl_spinner",
                   "mir_demo_client_wayland", "mir_demo_client_wayland_egl_spinner"});
    std::this_thread::sleep_for(10s);

    auto const statistics = report->latest_statistics();
    ASSERT_TRUE(statistics) << "No compositor performance sample received";
    auto const compositor_fps = statistics->frames / std::chrono::duration<double>(statistics->elapsed).count();
    auto const compositor_render_time =
        std::chrono::duration<double, std::milli>(statistics->render_time).count() / statistics->frames;
    RecordProperty("framerate", std::to_string(compositor_fps));
    RecordProperty("render_time", std::to_string(compositor_render_time));
    EXPECT_GE(compositor_fps, 0);
    EXPECT_GT(compositor_render_time, 0);
}
