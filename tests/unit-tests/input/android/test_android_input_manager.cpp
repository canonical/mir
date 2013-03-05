/*
 * Copyright © 2013 Canonical Ltd.
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
 */

#include "src/input/android/android_input_manager.h"
#include "src/input/android/android_input_configuration.h"

#include "mir/input/cursor_listener.h"

#include "mir_test_doubles/mock_viewable_area.h"
#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h" // TODO: Replace with mocked event hub ~ racarr

#include <EventHub.h>
#include <utils/StrongPointer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

// Test constants
namespace
{
static const std::shared_ptr<mi::CursorListener> null_cursor_listener{};
static std::initializer_list<std::shared_ptr<mi::EventFilter> const> empty_event_filters{};
static const geom::Rectangle default_view_area =
    geom::Rectangle{geom::Point(),
                    geom::Size{geom::Width(1600), geom::Height(1400)}};
}


namespace
{
struct MockInputConfiguration : public mia::InputConfiguration
{
    MOCK_METHOD0(the_event_hub, droidinput::sp<droidinput::EventHubInterface>());
};
}

TEST(AndroidInputManager, takes_event_hub_from_configuration)
{
    using namespace ::testing;
    
    MockInputConfiguration config;
    // TODO: Move to fixture
    mtd::MockViewableArea view_area;
    droidinput::sp<droidinput::EventHubInterface> event_hub = new mia::FakeEventHub(); // TODO: Replace with mock ~racarr

    ON_CALL(view_area, view_area())
        .WillByDefault(Return(default_view_area));
    EXPECT_CALL(config, the_event_hub()).Times(1).WillOnce(Return(event_hub));

    mia::InputManager(mt::fake_shared(config),
                      empty_event_filters,
                      mt::fake_shared(view_area),
                      null_cursor_listener);
                      
}
