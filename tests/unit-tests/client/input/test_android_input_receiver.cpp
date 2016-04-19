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

#include "src/client/input/android/android_input_receiver.h"
#include "src/server/input/channel.h"
#include "mir/input/null_input_receiver_report.h"
#include "mir/input/xkb_mapper.h"
#include "mir/events/event_private.h"

#include "mir_toolkit/event.h"

#include "mir/test/fd_utils.h"

#include <androidfw/InputTransport.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <poll.h>
#include <unistd.h>
#include <memory>
#include <system_error>
#include <boost/throw_exception.hpp>

namespace mircv = mir::input::receiver;
namespace mircva = mircv::android;
namespace md = mir::dispatch;
namespace mt = mir::test;
namespace mi = mir::input;

namespace droidinput = android;

namespace
{

class TestingInputProducer
{
public:
    TestingInputProducer(int fd)
        : input_publisher(std::make_shared<droidinput::InputPublisher>(new droidinput::InputChannel("", fd))),
          incrementing_seq_id(1),  // Sequence id must be > 0 or publisher will reject
          testing_key_event_scan_code(13)
    {
    }

    // The input publisher does not care about event semantics so we only highlight
    // a few fields for transport verification
    void produce_a_key_event()
    {
        input_publisher->publishKeyEvent(incrementing_seq_id,
                                         filler_device_id,
                                         0 /* source */,
                                         0 /* action */,
                                         0 /* flags */,
                                         0 /* key_code */,
                                         testing_key_event_scan_code,
                                         0 /* meta_state */,
                                         0 /* repeat_count */,
                                         {{}} /* mac */,
                                         std::chrono::nanoseconds(0) /* down_time */,
                                         std::chrono::nanoseconds(0) /* event_time */);
    }
    void produce_a_pointer_event(float x, float y, std::chrono::nanoseconds t)
    {
        droidinput::PointerProperties filler_pointer_properties;
        droidinput::PointerCoords filler_pointer_coordinates;

        memset(&filler_pointer_properties, 0, sizeof(droidinput::PointerProperties));
        memset(&filler_pointer_coordinates, 0, sizeof(droidinput::PointerCoords));

        filler_pointer_coordinates.setAxisValue(AMOTION_EVENT_AXIS_X, x);
        filler_pointer_coordinates.setAxisValue(AMOTION_EVENT_AXIS_Y, y);

        input_publisher->publishMotionEvent(incrementing_seq_id,
                                            filler_device_id,
                                            0 /* source */,
                                            motion_event_action_flags,
                                            0 /* flags */,
                                            0 /* edge_flags */,
                                            0 /* meta_state */,
                                            0 /* button_state */,
                                            0 /* x_offset */,
                                            0 /* y_offset */,
                                            0 /* x_precision */,
                                            0 /* y_precision */,
                                            {{}} /* mac */,
                                            std::chrono::nanoseconds(0) /* down_time */,
                                            t,
                                            default_pointer_count,
                                            &filler_pointer_properties,
                                            &filler_pointer_coordinates);
    }

    bool must_receive_handled_signal()
    {
        uint32_t seq;
        bool handled;

        auto status = input_publisher->receiveFinishedSignal(&seq, &handled);
        return (status == droidinput::OK) && handled;
    }

    std::shared_ptr<droidinput::InputPublisher> input_publisher;

    int incrementing_seq_id;
    int32_t testing_key_event_scan_code;

    // Some default values
    // device_id must be > 0 or input publisher will reject
    static const int32_t filler_device_id = 1;
    // event_action_move is necessary to engage batching behavior
    static const int32_t motion_event_action_flags = AMOTION_EVENT_ACTION_MOVE;
    // We have to have at least 1 pointer or the publisher will fail to marshal a motion event
    static const int32_t default_pointer_count = 1;
};

class AndroidInputReceiverSetup : public testing::Test
{
public:
    void flush_channels()
    {
        fsync(channel.server_fd());
        fsync(channel.client_fd());
    }

    mi::Channel channel;

    static std::chrono::milliseconds const next_event_timeout;
    std::function<void(MirEvent*)> event_handler{[](MirEvent*){}};
};

std::chrono::milliseconds const AndroidInputReceiverSetup::next_event_timeout(1000);
}

TEST_F(AndroidInputReceiverSetup, receiver_receives_key_events)
{
    MirKeyboardEvent last_event;
    mircva::InputReceiver receiver{channel.client_fd(),
                                   std::make_shared<mircv::XKBMapper>(),
                                   [&last_event](MirEvent* ev)
                                   {
                                       if (ev->type() == mir_event_type_key)
                                       {
                                           last_event = *ev->to_input()->to_keyboard();
                                       }
                                   },
                                   std::make_shared<mircv::NullInputReceiverReport>()};
    TestingInputProducer producer{channel.server_fd()};

    producer.produce_a_key_event();

    flush_channels();

    EXPECT_TRUE(mt::fd_becomes_readable(receiver.watch_fd(), next_event_timeout));
    receiver.dispatch(md::FdEvent::readable);

    EXPECT_EQ(mir_event_type_key, last_event.type());
    EXPECT_EQ(producer.testing_key_event_scan_code, last_event.scan_code());
}

TEST_F(AndroidInputReceiverSetup, receiver_handles_events)
{
    mircva::InputReceiver receiver{channel.client_fd(),
                                   std::make_shared<mircv::XKBMapper>(),
                                   event_handler,
                                   std::make_shared<mircv::NullInputReceiverReport>()};
    TestingInputProducer producer(channel.server_fd());

    producer.produce_a_key_event();
    flush_channels();

    EXPECT_TRUE(mt::fd_becomes_readable(receiver.watch_fd(), next_event_timeout));
    receiver.dispatch(md::FdEvent::readable);

    flush_channels();

    EXPECT_TRUE(producer.must_receive_handled_signal());
}

TEST_F(AndroidInputReceiverSetup, receiver_consumes_batched_motion_events)
{
    mircva::InputReceiver receiver{channel.client_fd(),
                                   std::make_shared<mircv::XKBMapper>(),
                                   event_handler,
                                   std::make_shared<mircv::NullInputReceiverReport>()};

    TestingInputProducer producer(channel.server_fd());

    // Produce 3 motion events before client handles any.
    producer.produce_a_pointer_event(0, 0, std::chrono::nanoseconds(0));
    producer.produce_a_pointer_event(0, 0, std::chrono::nanoseconds(0));
    producer.produce_a_pointer_event(0, 0, std::chrono::nanoseconds(0));

    flush_channels();

    EXPECT_TRUE(mt::fd_becomes_readable(receiver.watch_fd(), next_event_timeout));
    receiver.dispatch(md::FdEvent::readable);

    // Now there should be no events
    EXPECT_FALSE(mt::fd_is_readable(receiver.watch_fd()));
}

TEST_F(AndroidInputReceiverSetup, slow_raw_input_doesnt_cause_frameskipping)
{  // Regression test for LP: #1372300
    using namespace testing;
    using namespace std::chrono;
    using namespace std::literals::chrono_literals;

    auto t = 0ns;

    std::unique_ptr<MirEvent> ev;
    bool handler_called{false};

    mircva::InputReceiver receiver{channel.client_fd(),
                                   std::make_shared<mircv::XKBMapper>(),
                                   [&ev, &handler_called](MirEvent* event)
                                   {
                                       ev.reset(new MirEvent(*event));
                                       handler_called = true;
                                   },
                                   std::make_shared<mircv::NullInputReceiverReport>(),
                                   [&t](int)
                                   {
                                       return t;
                                   }};
    TestingInputProducer producer(channel.server_fd());

    nanoseconds const one_frame = duration_cast<nanoseconds>(1s) / 60;

    producer.produce_a_pointer_event(123, 456, t);
    producer.produce_a_key_event();
    flush_channels();

    // Key events don't get resampled. Will be reported first.
    EXPECT_TRUE(mt::fd_becomes_readable(receiver.watch_fd(), next_event_timeout));
    receiver.dispatch(md::FdEvent::readable);
    EXPECT_TRUE(handler_called);
    ASSERT_EQ(mir_event_type_key, ev->type());

    // The motion is still too new. Won't be reported yet, but is batched.
    auto start = high_resolution_clock::now();

    EXPECT_TRUE(mt::fd_becomes_readable(receiver.watch_fd(), 1ms));
    handler_called = false;
    receiver.dispatch(md::FdEvent::readable);
    // We've processed the data, but no new event has been generated.
    EXPECT_FALSE(handler_called);

    auto end = high_resolution_clock::now();
    auto duration = end - start;

    // Verify we timed out in under a frame (LP: #1372300)
    // Sadly using real time as droidinput::Looper doesn't use a mocked clock.
    ASSERT_LT(duration, one_frame);

    // Verify we don't use all the CPU by not sleeping (LP: #1373809)
    EXPECT_GT(duration, 1ms);

    // But later in a frame or so, the motion will be reported:
    t += 2 * one_frame;  // Account for the slower 59Hz event rate
    EXPECT_TRUE(mt::fd_becomes_readable(receiver.watch_fd(), next_event_timeout));
    receiver.dispatch(md::FdEvent::readable);

    EXPECT_TRUE(handler_called);
    EXPECT_EQ(mir_event_type_motion, ev->type());
}

TEST_F(AndroidInputReceiverSetup, finish_signalled_after_handler)
{
    bool handler_called = false;

    TestingInputProducer producer(channel.server_fd());
    mircva::InputReceiver receiver{
        channel.client_fd(),
        std::make_shared<mircv::XKBMapper>(),
        [&handler_called, &producer](MirEvent*)
        {
            EXPECT_FALSE(producer.must_receive_handled_signal());
            handler_called = true;
        },
        std::make_shared<mircv::NullInputReceiverReport>()
    };

    producer.produce_a_key_event();
    flush_channels();
    receiver.dispatch(md::FdEvent::readable);
    EXPECT_TRUE(handler_called);
    EXPECT_TRUE(producer.must_receive_handled_signal());
}

TEST_F(AndroidInputReceiverSetup, rendering_does_not_lag_behind_input)
{
    using namespace testing;
    using namespace std::chrono;
    using namespace std::literals::chrono_literals;

    std::chrono::nanoseconds t;

    int frames_triggered = 0;

    mircva::InputReceiver receiver{channel.client_fd(),
                                   std::make_shared<mircv::XKBMapper>(),
                                   [&frames_triggered](MirEvent*)
                                   {
                                       ++frames_triggered;
                                   },
                                   std::make_shared<mircv::NullInputReceiverReport>(),
                                   [&t](int)
                                   {
                                       return t;
                                   }};
    TestingInputProducer producer(channel.server_fd());

    std::chrono::nanoseconds const device_sample_interval = duration_cast<nanoseconds>(1s) / 250;
    std::chrono::nanoseconds const frame_interval = duration_cast<nanoseconds>(1s) / 60;
    std::chrono::nanoseconds const gesture_duration = 1s;

    std::chrono::nanoseconds last_produced = 0ns;

    for (t = 0ns; t < gesture_duration; t += 1ms)
    {
        if (!t.count() || t >= (last_produced + device_sample_interval))
        {
            last_produced = t;
            float a = t.count() * M_PI / 1000000.0f;
            float x = 500.0f * sinf(a);
            float y = 1000.0f * cosf(a);
            producer.produce_a_pointer_event(x, y, t);
            flush_channels();
        }

        if (mt::fd_is_readable(receiver.watch_fd()))
        {
            receiver.dispatch(md::FdEvent::readable);
        }
    }

    // If the rendering time resulting from the gesture is longer than the
    // gesture itself then that's laggy...
    std::chrono::nanoseconds render_duration = frame_interval * frames_triggered;
    EXPECT_THAT(render_duration, Le(gesture_duration));

    int average_lag_milliseconds = (render_duration - gesture_duration) / (frames_triggered * 1ms);
    EXPECT_THAT(average_lag_milliseconds, Le(1));
}

TEST_F(AndroidInputReceiverSetup, input_comes_in_phase_with_rendering)
{
    using namespace testing;

    std::chrono::nanoseconds t;

    mircva::InputReceiver receiver{
        channel.client_fd(),
        std::make_shared<mircv::XKBMapper>(),
        event_handler,
        std::make_shared<mircv::NullInputReceiverReport>(),
    };
    TestingInputProducer producer(channel.server_fd());

    std::chrono::nanoseconds const one_millisecond = std::chrono::nanoseconds(1000000ULL);
    std::chrono::nanoseconds const one_second = 1000 * one_millisecond;
    std::chrono::nanoseconds const device_sample_interval = one_second / 125;
    std::chrono::nanoseconds const frame_interval = one_second / 60;
    std::chrono::nanoseconds const gesture_duration = 10 * one_second;

    std::chrono::nanoseconds last_produced = std::chrono::nanoseconds(0);
    std::chrono::nanoseconds last_consumed = std::chrono::nanoseconds(0);
    std::chrono::nanoseconds last_rendered = std::chrono::nanoseconds(0);
    std::chrono::nanoseconds last_in_phase = std::chrono::nanoseconds(0);

    for (t = std::chrono::nanoseconds(0); t < gesture_duration; t += one_millisecond)
    {
        if (!t.count() || t >= (last_produced + device_sample_interval))
        {
            last_produced = t;
            float a = t.count() * M_PI / 1000000.0f;
            float x = 500.0f * sinf(a);
            float y = 1000.0f * cosf(a);
            producer.produce_a_pointer_event(x, y, t);
            flush_channels();
        }

        if (mt::fd_is_readable(receiver.watch_fd()))
        {
            receiver.dispatch(md::FdEvent::readable);
            last_consumed = t;
        }

        if (t >= (last_rendered + frame_interval))
        {
            last_rendered = t;

            if (last_consumed.count())
            {
                auto lag = last_rendered - last_consumed;

                // How often does vsync drift in phase (very close) with the
                // time that we emitted/consumed input events?
                if (lag < 4 * one_millisecond)
                    last_in_phase = t;
                last_consumed = std::chrono::nanoseconds(0);
            }
        }

        // Verify input and vsync come into phase at least a few times every
        // second (if not always). This ensure visible lag is minimized.
        ASSERT_GE(250 * one_millisecond, (t - last_in_phase));
    }
}
