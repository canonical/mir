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

#include "mir_test/fake_event_hub.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/semaphore.h"

// Needed implicitly for InputManager destructor because of android::sp :/
#include <InputDispatcher.h>
#include <InputReader.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

#include <semaphore.h>
#include <time.h>

namespace mi = mir::input;
namespace mia = mir::input::android;

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    MOCK_METHOD1(handles, bool(const android::InputEvent*));
};
}

TEST(AndroidInputManagerAndEventFilterDispatcherPolicy, manager_dispatches_to_filter)
{
    using namespace ::testing;
    android::sp<mia::FakeEventHub> event_hub = new mia::FakeEventHub();
    MockEventFilter event_filter;
    mia::InputManager input_manager(event_hub, {std::shared_ptr<mi::EventFilter>(&event_filter, mir::EmptyDeleter())});
    mir::Semaphore semaphore;
    
    EXPECT_CALL(event_filter, handles(_)).WillOnce(DoAll(PostSemaphore(&semaphore), Return(false)));    
    event_hub->synthesize_builtin_keyboard_added();
    event_hub->synthesize_key_event(KEY_ENTER);

    input_manager.start();
    semaphore.wait_seconds(5);
    input_manager.stop();
}
