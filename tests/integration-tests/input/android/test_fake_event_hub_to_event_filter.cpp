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
#include "mir/thread/all.h"

#include "mir_test/empty_deleter.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/mock_event_filter.h"
#include "mir_test/wait_condition.h"

#include <InputDispatcher.h>
#include <InputReader.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mia = mi::android;

namespace mir
{

class FakeEventHubSetup : public testing::Test
{
  public:
    FakeEventHubSetup()
    {
    }

    ~FakeEventHubSetup()
    {
    }

    void SetUp()
    {
        event_hub = new mia::FakeEventHub();
        dispatcher_policy = new mia::EventFilterDispatcherPolicy(
            std::shared_ptr<mi::EventFilter>(&event_filter, mir::EmptyDeleter()));
        reader_policy = new mia::DummyInputReaderPolicy();
        dispatcher = new droidinput::InputDispatcher(dispatcher_policy);
        reader = new droidinput::InputReader(event_hub, reader_policy, dispatcher);
        reader_thread = new droidinput::InputReaderThread(reader);
        dispatcher_thread = new droidinput::InputDispatcherThread(dispatcher);

        dispatcher->setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen);
        dispatcher->setInputFilterEnabled(true);

        dispatcher_thread->run("InputDispatcher", droidinput::PRIORITY_URGENT_DISPLAY);
        reader_thread->run("InputReader", droidinput::PRIORITY_URGENT_DISPLAY);
    }

    void TearDown()
    {
        reader_thread->requestExitAndWait();
        dispatcher_thread->requestExitAndWait();
    }

  protected:
    MockEventFilter event_filter;
    droidinput::sp<mia::FakeEventHub> event_hub;
    droidinput::sp<droidinput::InputDispatcherPolicyInterface> dispatcher_policy;
    droidinput::sp<droidinput::InputReaderPolicyInterface> reader_policy;
    droidinput::sp<droidinput::InputDispatcher> dispatcher;
    droidinput::sp<droidinput::InputReader> reader;
    droidinput::sp<droidinput::InputReaderThread> reader_thread;
    droidinput::sp<droidinput::InputDispatcherThread> dispatcher_thread;
};

}

using mir::FakeEventHubSetup;
using mir::WaitCondition;

ACTION_P(ReturnFalseAndWakeUp, wait_condition)
{
    wait_condition->wake_up_everyone();
    return false;
}

TEST_F(FakeEventHubSetup, fake_event_hub_dispatches_to_filter)
{
    using namespace ::testing;

    static const int key = KEY_ENTER;

    mir::WaitCondition wait_condition;

    EXPECT_CALL(event_filter, handles(IsKeyEventWithKey(key))).Times(1)
            .WillOnce(ReturnFalseAndWakeUp(&wait_condition));

    event_hub->synthesize_builtin_keyboard_added();
    event_hub->synthesize_key_event(key);

    wait_condition.wait_for_seconds(30);
}
