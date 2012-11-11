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
#include "mir/input/event_filter.h"
#include "src/input/android/android_input_manager.h"
#include "mir/thread/all.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/empty_deleter.h"

// Needed implicitly for InputManager destructor because of droidinput::sp :/
#include <InputDispatcher.h>
#include <InputReader.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

namespace mi = mir::input;
namespace mia = mir::input::android;

namespace
{
    using namespace ::testing;
    MATCHER_P(EventWithKeycode, keycode, std::string(negation ? "does match" : "does not match") + PrintToString(keycode))
    {
	auto key_ev = static_cast<const droidinput::KeyEvent*>(arg);
	return (keycode == key_ev->getKeyCode());
    }
}

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    MOCK_METHOD1(handles, bool(const droidinput::InputEvent*));
};

struct KeycodeMatchingEventFilter : public mi::EventFilter
{
    KeycodeMatchingEventFilter(int keycode) : keycode(keycode) {}
    virtual ~KeycodeMatchingEventFilter() {}
    bool handles(const droidinput::InputEvent *event) 
    {
	EXPECT_EQ(static_cast<const droidinput::KeyEvent*>(event)->getKeyCode(), keycode);
	return true;
    }
    int keycode;
    };

}

TEST(AndroidInputManagerAndEventFilterDispatcherPolicy, manager_dispatches_to_filter)
{
    using namespace ::testing;
    droidinput::sp<mia::FakeEventHub> event_hub = new mia::FakeEventHub();
    MockEventFilter event_filter;
    mia::InputManager input_manager(event_hub, {std::shared_ptr<mi::EventFilter>(&event_filter, mir::EmptyDeleter())});
    
    EXPECT_CALL(event_filter, handles(_)).WillOnce(Return(false));    
    event_hub->synthesize_builtin_keyboard_added();
    event_hub->synthesize_key_event();

    input_manager.start();
    // TODO: This timeout is very long, but threads are very slow in valgrind. 
    // Timeout will be improved in followup branch with semaphore/wait condition approach.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    input_manager.stop();
}

TEST(AndroidInputManagerAndEventFilterDispatcherPolicy, keys_are_mapped)
{
    using namespace ::testing;
    droidinput::sp<mia::FakeEventHub> event_hub = new mia::FakeEventHub();
    MockEventFilter event_filter;
    mia::InputManager input_manager(event_hub, {std::shared_ptr<mi::EventFilter>(&event_filter, mir::EmptyDeleter())});
      
    EXPECT_CALL(event_filter, handles(EventWithKeycode((int)AKEYCODE_DPAD_RIGHT))).
        WillOnce(Return(false));
    
    event_hub->synthesize_builtin_keyboard_added();
    event_hub->synthesize_key_event(KEY_RIGHT);

    input_manager.start();
    // TODO: This timeout is very long, but threads are very slow in valgrind. 
    // Timeout will be improved in followup branch with semaphore/wait condition approach.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    input_manager.stop();
}
