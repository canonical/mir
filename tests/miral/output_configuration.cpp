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

#include <miral/output_configuration.h>

#include <miral/test_server.h>

#include <mir/graphics/display.h>
#include <mir/server.h>

#include <gmock/gmock.h>

#include <atomic>

using namespace testing;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
auto const test_scale{2.0f};
auto const test_position{geom::Point{100, 200}};

struct TestStrategy : miral::OutputConfiguration::Strategy
{
    void apply_configuration(std::span<mg::UserDisplayConfigurationOutput> outputs) override
    {
        applied_outputs = outputs.size();

        for (auto& output : outputs)
        {
            output.scale = test_scale;
            output.top_left = test_position;
        }
    }

    void confirm_configuration(std::span<mg::UserDisplayConfigurationOutput const> outputs) override
    {
        confirmed_outputs = outputs.size();
    }

    std::atomic<size_t> applied_outputs{0};
    std::atomic<size_t> confirmed_outputs{0};
};

struct OutputConfigurationTest : miral::TestServer
{
    OutputConfigurationTest()
    {
        add_server_init([this](mir::Server& server) { output_configuration(server); });
    }

    std::shared_ptr<TestStrategy> const strategy{std::make_shared<TestStrategy>()};
    miral::OutputConfiguration output_configuration{strategy};
};
}

TEST_F(OutputConfigurationTest, strategy_sees_the_outputs)
{
    EXPECT_THAT(strategy->applied_outputs.load(), Gt(0));
    EXPECT_THAT(strategy->confirmed_outputs.load(), Eq(strategy->applied_outputs.load()));
}

TEST_F(OutputConfigurationTest, changes_made_by_the_strategy_reach_the_display_configuration)
{
    auto const configuration = server().the_display()->configuration();

    auto outputs{0};

    configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            ++outputs;
            EXPECT_THAT(output.scale, Eq(test_scale));
            EXPECT_THAT(output.top_left, Eq(test_position));
        });

    EXPECT_THAT(outputs, Gt(0));
}
