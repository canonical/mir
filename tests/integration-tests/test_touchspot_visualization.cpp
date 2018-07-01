/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/touch_visualizer.h"

#include "mir/input/device_capability.h"
#include "mir/input/input_device_info.h"

#include "mir/test/barrier.h"
#include "mir/test/fake_shared.h"

#include "mir_test_framework/deferred_in_process_server.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/stub_server_platform_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <functional>

namespace mi = mir::input;
namespace mis = mi::synthesis;
namespace mt = mir::test;
namespace geom = mir::geometry;
namespace mtf = mir_test_framework;

namespace
{

ACTION_P(UnblockBarrier, barrier)
{
    barrier->ready();
}

MATCHER(NoSpots, "")
{
    if (arg.size() > 0)
        return false;
    return true;
}

static bool
spots_have_position_and_pressure(std::vector<mi::TouchVisualizer::Spot> const& spots,
    std::vector<geom::Point> const& positions, float pressure)
{
    if (spots.size() != positions.size())
        return false;
    auto spots_it = spots.begin();
    auto positions_it = positions.begin();

    while (spots_it != spots.end())
    {
        auto const& spot = *spots_it;
        if (spot.touch_location != *positions_it)
            return false;
        if (spot.pressure != pressure)
            return false;
        spots_it++;
        positions_it++;
    }
    return true;
}

MATCHER_P(TouchedSpotsAt, positions, "")
{
    return spots_have_position_and_pressure(arg, positions, 1.0);
}

struct MockTouchVisualizer : public mi::TouchVisualizer
{
    MOCK_METHOD1(visualize_touches, void(std::vector<mi::TouchVisualizer::Spot> const&));
    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
};

struct ServerConfiguration : mtf::InputTestingServerConfiguration
{
    mt::Barrier& test_complete;

    MockTouchVisualizer mock_visualizer;
    static geom::Rectangle const display_bounds;    
    
    ServerConfiguration(mt::Barrier& test_complete)
        : InputTestingServerConfiguration({display_bounds}),
          test_complete(test_complete)
    {
    }
    void inject_input() override{}
    
    std::shared_ptr<mi::TouchVisualizer> the_touch_visualizer() override
    {
        return mt::fake_shared(mock_visualizer);
    }
};

geom::Rectangle const ServerConfiguration::display_bounds = {{0, 0}, {1600, 1600}};

struct TestTouchspotVisualizations : mtf::DeferredInProcessServer
{
    mt::Barrier test_complete_fence{2};
    int env_setup = setenv("MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str(), 1);
    ServerConfiguration server_configuration{test_complete_fence};

    std::unique_ptr<mtf::FakeInputDevice> fake_touch_screen{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"ts", "ts-uid" , mi::DeviceCapability::touchscreen|mi::DeviceCapability::multitouch})
        };


    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }

    void start_server()
    {
        DeferredInProcessServer::start_server();
        server_configuration.exec();
    }

    void TearDown() override
    {
        test_complete_fence.ready();
        server_configuration.on_exit();
        DeferredInProcessServer::TearDown();
    }
};

}

using namespace ::testing;

TEST_F(TestTouchspotVisualizations, touch_is_given_to_touchspot_visualizer)
{
    geom::Point const abs_touch{0, 0};
    std::vector<geom::Point> const expected_spots{abs_touch};

    InSequence seq;
    // First we will see the spots cleared, as this is the start of a new gesture.
    EXPECT_CALL(server_configuration.mock_visualizer, visualize_touches(NoSpots())).Times(AtMost(1));
    EXPECT_CALL(server_configuration.mock_visualizer, visualize_touches(TouchedSpotsAt(expected_spots)));
    EXPECT_CALL(server_configuration.mock_visualizer, visualize_touches(NoSpots())).Times(1).
        WillOnce(UnblockBarrier(&server_configuration.test_complete));

    start_server();

    fake_touch_screen->emit_event(mis::a_touch_event().at_position(abs_touch));
    fake_touch_screen->emit_event(mis::a_touch_event().at_position(abs_touch)
                                  .with_action(mis::TouchParameters::Action::Release));
}

TEST_F(TestTouchspotVisualizations, touchspots_follow_gesture)
{
    auto const display_size{ServerConfiguration::display_bounds.size};
    geom::Point const abs_touch{0, 0};
    geom::Point const abs_touch_2{display_size.width.as_uint32_t() - 1,
                                   display_size.height.as_uint32_t() - 1};
    std::vector<geom::Point> expected_spots_1{abs_touch};
    std::vector<geom::Point> expected_spots_2{abs_touch_2};

    InSequence seq;

    EXPECT_CALL(server_configuration.mock_visualizer, visualize_touches(NoSpots())).Times(AtMost(1));
    EXPECT_CALL(server_configuration.mock_visualizer, visualize_touches(TouchedSpotsAt(expected_spots_1))).Times(1);
    EXPECT_CALL(server_configuration.mock_visualizer, visualize_touches(TouchedSpotsAt(expected_spots_2))).Times(1);
    EXPECT_CALL(server_configuration.mock_visualizer, visualize_touches(NoSpots())).
                Times(1).WillOnce(UnblockBarrier(&server_configuration.test_complete));

    start_server();

    fake_touch_screen->emit_event(mis::a_touch_event().at_position(abs_touch));
    fake_touch_screen->emit_event(mis::a_touch_event().at_position(abs_touch_2)
        .with_action(mis::TouchParameters::Action::Move));
    fake_touch_screen->emit_event(mis::a_touch_event().at_position(abs_touch_2)
        .with_action(mis::TouchParameters::Action::Release));
}
