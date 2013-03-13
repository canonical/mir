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
#include "src/server/input/android/event_filter_dispatcher_policy.h"
#include "src/server/input/android/rudimentary_input_reader_policy.h"
#include "src/server/input/android/android_input_constants.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test/wait_condition.h"
#include "mir_test/event_factory.h"

#include <InputDispatcher.h>
#include <InputReader.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mir::input::synthesis;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

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
        dispatcher_policy = new mia::EventFilterDispatcherPolicy(mt::fake_shared(event_filter));
        reader_policy = new mia::RudimentaryInputReaderPolicy();
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
        dispatcher_thread->requestExit();
        dispatcher->setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen);
        dispatcher_thread->join();

        reader_thread->requestExit();
        event_hub->wake();
        reader_thread->join();
    }

  protected:
    mtd::MockEventFilter event_filter;
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

TEST_F(FakeEventHubSetup, fake_event_hub_dispatches_to_filter)
{
    using namespace ::testing;

    mir::WaitCondition wait_condition;

    EXPECT_CALL(event_filter, handles(KeyDownEvent())).Times(1)
        .WillOnce(ReturnFalseAndWakeUp(&wait_condition));

    event_hub->synthesize_builtin_keyboard_added();
    event_hub->synthesize_device_scan_complete();

    event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_ENTER));

    wait_condition.wait_for_at_most_seconds(1);
}
