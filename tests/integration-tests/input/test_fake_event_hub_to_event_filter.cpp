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

#include <InputDispatcher.h>
#include <InputReader.h>

#include "mir/input/event_filter.h"
#include "mir/input/event_filter_dispatcher_policy.h"

#include "mir_test/fake_event_hub.cpp"
#include "mir_test/empty_delete.h"

namespace mi = mir::input;

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    MOCK_METHOD1(filter_event, bool(const android::InputEvent*));
};
}

TEST(InputDispatcherAndEventFilterDispatcherPolicy, fake_event_hub_dispatches_to_filter)
{
    using namespace ::testing;
    android::sp<FakeEventHub> event_hub = new FakeEventHub();
    MockEventFilter event_filter;
    android::sp<android::InputDispatcherPolicyInterface> dispatcher_policy = new mi::EventFilterDispatcherPolicy(std::shared_ptr<mi::EventFilter>(&event_filter, mir::EmptyDeleter()));
    android::sp<android::InputReaderPolicyInterface> reader_policy = new mi::DummyInputReaderPolicy();
    auto dispatcher = new InputDispatcher(dispatcher_policy);
    auto reader = new InputReader(event_hub, reader_policy, dispatcher);
    auto reader_thread = new InputReaderThread(reader);
    auto dispatcher_thread = new InputDispatcherThread(dispatcher);
    
    EXPECT_CALL(event_filter, filter_event(_)).Times(1).WillOnce(Return(f
alse));    
    
    event_hub.synthesize_builtin_keyboard_adde();
    event_hub.synthesize_key_event(1);
    
    dispacher->setInputDispatchMode(true, false);
    dispatcher->setInputFilterEnabled(true);
    dispatcher_thread->run("InputDispatcher", android::PRIORITY_URGENT_DISPLAY);
    reader_thread->run("InputReader", android::PRIORITY_URGENT_DISPLAY);
                       
                       
}
