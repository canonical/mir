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
#include "src/input/android/event_filter_dispatcher_policy.h"
#include "src/input/android/dummy_input_reader_policy.h"
#include "src/input/android/android_input_constants.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/semaphore.h"

#include <InputDispatcher.h>
#include <InputReader.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mia = mi::android;

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    MOCK_METHOD1(handles, bool(const droidinput::InputEvent*));
};
}

TEST(InputDispatcherAndEventFilterDispatcherPolicy, fake_event_hub_dispatches_to_filter)
{
    using namespace ::testing;
    droidinput::sp<mia::FakeEventHub> event_hub = new mia::FakeEventHub();
    MockEventFilter event_filter;
    droidinput::sp<droidinput::InputDispatcherPolicyInterface> dispatcher_policy = new mia::EventFilterDispatcherPolicy(std::shared_ptr<mi::EventFilter>(&event_filter, mir::EmptyDeleter()));
    droidinput::sp<droidinput::InputReaderPolicyInterface> reader_policy = new mia::DummyInputReaderPolicy();
    auto dispatcher = new droidinput::InputDispatcher(dispatcher_policy);
    auto reader = new droidinput::InputReader(event_hub, reader_policy, dispatcher);
    droidinput::sp<droidinput::InputReaderThread> reader_thread = new droidinput::InputReaderThread(reader);
    droidinput::sp<droidinput::InputDispatcherThread> dispatcher_thread = new droidinput::InputDispatcherThread(dispatcher);
    
    mir::Semaphore event_handled;
    EXPECT_CALL(event_filter, handles(_)).Times(1).WillOnce(DoAll(PostSemaphore(&event_handled), Return(false)));    
    
    dispatcher->setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen);
    dispatcher->setInputFilterEnabled(true);

    event_hub->synthesize_builtin_keyboard_added();
    event_hub->synthesize_key_event();

    dispatcher_thread->run("InputDispatcher", droidinput::PRIORITY_URGENT_DISPLAY);
    reader_thread->run("InputReader", droidinput::PRIORITY_URGENT_DISPLAY);

    event_handled.wait_seconds(5);

    reader_thread->requestExitAndWait();
    dispatcher_thread->requestExitAndWait();
}
