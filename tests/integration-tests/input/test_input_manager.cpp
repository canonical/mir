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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mir/thread/all.h"

// Needed implicitly for InputManager destructor because of android::sp :/
#include <InputDispatcher.h>
#include <InputReader.h>

#include "mir/input/event_filter.h"
#include "mir/input/input_manager.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/empty_deleter.h"

namespace mi = mir::input;

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    MOCK_METHOD1(handles, bool(const android::InputEvent*));
};
}

TEST(InputManagerAndEventFilterDispatcherPolicy, fake_event_hub_dispatches_to_filter)
{
    using namespace ::testing;
    android::sp<mir::FakeEventHub> event_hub = new mir::FakeEventHub();
    MockEventFilter event_filter;
    mi::InputManager input_manager;
    
    EXPECT_CALL(event_filter, handles(_)).Times(1).WillOnce(Return(false));    
    event_hub->synthesize_builtin_keyboard_added();
    event_hub->synthesize_key_event(1);
    
    input_manager.add_filter(std::shared_ptr<MockEventFilter>(&event_filter, mir::EmptyDeleter()));                             

    input_manager.start(event_hub);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    input_manager.stop();
}
