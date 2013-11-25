/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "src/shared/input/android/android_input_receiver_thread.h"
#include "src/shared/input/android/android_input_receiver.h"

#include "mir/input/null_input_receiver_report.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <androidfw/InputTransport.h>

#include <atomic>

#include <fcntl.h>

namespace mircv = mir::input::receiver;
namespace mircva = mircv::android;

namespace
{

struct MockEventHandler
{
    MOCK_METHOD1(handle_event, void(MirEvent*));
};

struct MockInputReceiver : public mircva::InputReceiver
{
    MockInputReceiver(int fd)
      : InputReceiver(fd, std::make_shared<mircv::NullInputReceiverReport>())
    {
    }
    MOCK_METHOD1(next_event, bool(MirEvent &));
};

struct AndroidInputReceiverThreadSetup : public testing::Test
{
    AndroidInputReceiverThreadSetup()
    {
        test_receiver_fd = open("/dev/null", O_APPEND);
        input_receiver = std::make_shared<MockInputReceiver>(test_receiver_fd);
    }
    virtual ~AndroidInputReceiverThreadSetup()
    {
        close(test_receiver_fd);
    }

    int test_receiver_fd;
    std::shared_ptr<MockInputReceiver> input_receiver;
};

ACTION_P(StopThread, thread)
{
    thread->stop();
//    thread->join();
}

}

TEST_F(AndroidInputReceiverThreadSetup, reads_events_until_stopped)
{
    using namespace ::testing;

    mircva::InputReceiverThread input_thread(input_receiver,
        std::function<void(MirEvent*)>());
    {
        InSequence seq;
        EXPECT_CALL(*input_receiver, next_event(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*input_receiver, next_event(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*input_receiver, next_event(_)).Times(1).WillOnce(DoAll(StopThread(&input_thread), Return(false)));
    }
    input_thread.start();
    input_thread.join();
}

TEST_F(AndroidInputReceiverThreadSetup, receives_and_dispatches_available_events_when_ready)
{
    using namespace ::testing;
    MockEventHandler mock_handler;

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

    mircva::InputReceiverThread input_thread(input_receiver, input_delegate);
    {
        InSequence seq;

        EXPECT_CALL(*input_receiver, next_event(_)).Times(1).WillOnce(Return(true));
        EXPECT_CALL(*input_receiver, next_event(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*input_receiver, next_event(_)).Times(1).WillOnce(DoAll(StopThread(&input_thread), Return(true)));
    }
    EXPECT_CALL(mock_handler, handle_event(_)).Times(2);

    input_thread.start();
    input_thread.join();
}

TEST_F(AndroidInputReceiverThreadSetup, input_callback_invoked_from_thread)
{
    using namespace ::testing;
    MockEventHandler mock_handler;
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

    mircva::InputReceiverThread input_thread(input_receiver, input_delegate);
    {
        InSequence seq;

        EXPECT_CALL(*input_receiver, next_event(_)).Times(1).WillOnce(DoAll(StopThread(&input_thread), Return(true)));
    }

    input_thread.start();

    // We would block forever here were delivery not threaded.
    // Yield to improve the run-time under valgrind.
    while (handled == false)
        std::this_thread::yield();

    input_thread.join();
}

