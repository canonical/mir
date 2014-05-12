/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 *              Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/input_dispatcher_configuration.h"
#include "src/server/report/null_report_factory.h"

#include "mir/input/android/default_android_input_configuration.h"
#include "mir/input/event_filter.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/input_targets.h"
#include "mir/input/input_region.h"
#include "mir/input/input_dispatcher_configuration.h"
#include "mir/input/input_dispatcher.h"
#include "mir/geometry/rectangle.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test_doubles/stub_input_targets.h"
#include "mir_test_doubles/stub_scene.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test/wait_condition.h"
#include "mir_test/event_factory.h"

#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mis = mir::input::synthesis;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;

using mtd::MockEventFilter;

namespace
{
using namespace ::testing;

struct StubInputRegion : public mi::InputRegion
{
    geom::Rectangle bounding_rectangle()
    {
        return {geom::Point{}, geom::Size{1024, 1024}};
    }

    void confine(geom::Point&)
    {
    }
};

struct MockCursorListener : public mi::CursorListener
{
    MOCK_METHOD2(cursor_moved_to, void(float, float));

    ~MockCursorListener() noexcept {}
};

struct AndroidInputManagerAndCursorListenerSetup : public testing::Test
{
    void SetUp()
    {
        event_filter = std::make_shared<MockEventFilter>();
        dispatcher_conf = std::make_shared<mia::InputDispatcherConfiguration>(
            event_filter, mr::null_input_report(), mt::fake_shared(stub_scene), mt::fake_shared(stub_targets));
        configuration = std::make_shared<mtd::FakeEventHubInputConfiguration>(
            dispatcher_conf, mt::fake_shared(input_region), mt::fake_shared(cursor_listener), mr::null_input_report());

        fake_event_hub = configuration->the_fake_event_hub();

        input_manager = configuration->the_input_manager();
        dispatcher = dispatcher_conf->the_input_dispatcher();


        input_manager->start();
        dispatcher->start();
    }

    void TearDown()
    {
        dispatcher->stop();
        input_manager->stop();
    }

    std::shared_ptr<mia::InputDispatcherConfiguration> dispatcher_conf;
    std::shared_ptr<mtd::FakeEventHubInputConfiguration> configuration;
    mia::FakeEventHub* fake_event_hub;
    std::shared_ptr<MockEventFilter> event_filter;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<mi::InputDispatcher> dispatcher;
    MockCursorListener cursor_listener;
    mtd::StubInputTargets stub_targets;
    mtd::StubScene stub_scene;
    StubInputRegion input_region;
};

}


TEST_F(AndroidInputManagerAndCursorListenerSetup, cursor_listener_receives_motion)
{
    using namespace ::testing;

    auto wait_condition = std::make_shared<mt::WaitCondition>();

    static const float x = 100.f;
    static const float y = 100.f;

    EXPECT_CALL(cursor_listener, cursor_moved_to(x, y)).Times(1);

    // The stack doesn't like shutting down while events are still moving through
    EXPECT_CALL(*event_filter, handle(_))
            .WillOnce(mt::ReturnFalseAndWakeUp(wait_condition));

    fake_event_hub->synthesize_builtin_cursor_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(x, y));

    wait_condition->wait_for_at_most_seconds(1);
}
