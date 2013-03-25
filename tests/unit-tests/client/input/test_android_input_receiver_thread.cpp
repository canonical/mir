/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/client/input/android_input_receiver_thread.h"
#include "src/client/input/android_input_receiver.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <androidfw/InputTransport.h>

#include <atomic>

namespace mclia = mir::client::input::android;
namespace mt = mir::test;

namespace
{

struct MockEventHandler 
{
    MOCK_METHOD1(handle_event, void(MirEvent*));
};

struct MockInputReceiver : public mclia::InputReceiver
{
    MockInputReceiver()
      : InputReceiver(droidinput::sp<droidinput::InputChannel>()) // TODO: ? maybe initialize with null channel ~racarr
    {
    }
    MOCK_METHOD1(next_event, bool(MirEvent &));
    MOCK_METHOD1(poll, bool(std::chrono::milliseconds const&));
};

ACTION_P(StopThread, thread)
{
    thread->stop();
}

}

TEST(AndroidInputReceiverThread, polls_until_stopped)
{
    using namespace ::testing;

    MockInputReceiver input_receiver;
    mclia::InputReceiverThread input_thread(mt::fake_shared(input_receiver), 
                                           std::function<void(MirEvent*)>());    
    {
        InSequence seq;
        EXPECT_CALL(input_receiver, poll(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(input_receiver, poll(_)).Times(1).WillOnce(DoAll(StopThread(&input_thread), Return(false)));
    }
    input_thread.start();
    input_thread.join();
}

TEST(AndroidInputReceiverThread, receives_and_dispatches_available_events_when_ready)
{
    using namespace ::testing;
    MockEventHandler mock_handler;
    MockInputReceiver input_receiver;

    struct InputDelegate
    {
        InputDelegate(MockEventHandler& handler)
          : handler(handler) {}
        void operator()(MirEvent* ev)
        {
            handler.handle_event(ev);
        }
        MockEventHandler &handler;
    } input_delegate(mock_handler);

    mclia::InputReceiverThread input_thread(mt::fake_shared(input_receiver), input_delegate);
    {
        InSequence seq;

        EXPECT_CALL(input_receiver, poll(_)).Times(1).WillOnce(Return(true));
        EXPECT_CALL(input_receiver, next_event(_)).Times(1).WillOnce(Return(true));
        EXPECT_CALL(input_receiver, next_event(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(input_receiver, poll(_)).Times(1).WillOnce(DoAll(StopThread(&input_thread), Return(false)));
    }
    EXPECT_CALL(mock_handler, handle_event(_)).Times(1);

    input_thread.start();
    input_thread.join();
}

TEST(AndroidInputReceiverThread, input_callback_invoked_from_thread)
{
    using namespace ::testing;
    MockEventHandler mock_handler;
    MockInputReceiver input_receiver;
    std::atomic<bool> handled;
    
    handled = false;


    struct InputDelegate
    {
        InputDelegate(std::atomic<bool> &handled)
          : handled(handled) {}
        void operator()(MirEvent* /*ev*/)
        {
            handled = true;
        }
        std::atomic<bool> &handled;
    } input_delegate(handled);

    mclia::InputReceiverThread input_thread(mt::fake_shared(input_receiver), input_delegate);
    {
        InSequence seq;

        EXPECT_CALL(input_receiver, poll(_)).Times(1).WillOnce(Return(true));
        EXPECT_CALL(input_receiver, next_event(_)).Times(1).WillOnce(Return(true));
        EXPECT_CALL(input_receiver, next_event(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(input_receiver, poll(_)).Times(1).WillOnce(DoAll(StopThread(&input_thread), Return(false)));
    }

    input_thread.start();
    while (handled == false) { } // We would block forever here were delivery not threaded
    input_thread.join();
}

