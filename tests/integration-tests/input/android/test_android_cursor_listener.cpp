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

#include "src/server/input/event_filter_chain.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test_framework/fake_event_hub_server_configuration.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test_doubles/stub_input_enumerator.h"
#include "mir_test_doubles/stub_touch_visualizer.h"
#include "mir_test/wait_condition.h"
#include "mir_test/event_factory.h"

#include "mir/input/cursor_listener.h"
#include "mir/input/input_dispatcher.h"
#include "mir/input/input_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mis = mir::input::synthesis;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
using namespace ::testing;

struct MockCursorListener : public mi::CursorListener
{
    MOCK_METHOD2(cursor_moved_to, void(float, float));

    ~MockCursorListener() noexcept {}
};

struct AndroidCursorListenerIntegrationTest : testing::Test, mtf::FakeEventHubServerConfiguration
{
    bool is_key_repeat_enabled() const override
    {
        return false;
    }

    std::shared_ptr<mi::CompositeEventFilter> the_composite_event_filter() override
    {
        std::initializer_list<std::shared_ptr<mi::EventFilter>const> const& chain{std::static_pointer_cast<mi::EventFilter>(event_filter)};
        return std::make_shared<mi::EventFilterChain>(chain);
    }

    std::shared_ptr<mi::CursorListener> the_cursor_listener() override
    {
        return mt::fake_shared(cursor_listener);
    }

    void SetUp() override
    {
        configuration = the_input_configuration();

        input_manager = configuration->the_input_manager();
        input_manager->start();
        input_dispatcher = the_input_dispatcher();
        input_dispatcher->start();
    }

    void TearDown() override
    {
        input_dispatcher->stop();
        input_manager->stop();
    }

    MockCursorListener cursor_listener;
    std::shared_ptr<mtd::MockEventFilter> event_filter = std::make_shared<mtd::MockEventFilter>();
    std::shared_ptr<mi::InputConfiguration> configuration;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<mi::InputDispatcher> input_dispatcher;
};

}

TEST_F(AndroidCursorListenerIntegrationTest, cursor_listener_receives_motion)
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
