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

#include "src/common/input/android/android_input_receiver.h"
#include "mir/input/null_input_receiver_report.h"
#include "mir_toolkit/event.h"


#include <androidfw/InputTransport.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unistd.h>
#include <memory>

namespace mircv = mir::input::receiver;
namespace mircva = mircv::android;

namespace droidinput = android;

namespace
{

class TestingInputProducer
{
public:
    TestingInputProducer(int fd) :
        input_publisher(std::make_shared<droidinput::InputPublisher>(new droidinput::InputChannel("", fd))),
        incrementing_seq_id(1), // Sequence id must be > 0 or publisher will reject
        testing_key_event_scan_code(13)
    {
    }


    // The input publisher does not care about event semantics so we only highlight
    // a few fields for transport verification
    void produce_a_key_event()
    {
        input_publisher->publishKeyEvent(
            incrementing_seq_id,
            filler_device_id,
            0 /* source */,
            0 /* action */,
            0 /* flags */,
            0 /* key_code */,
            testing_key_event_scan_code,
            0 /* meta_state */,
            0 /* repeat_count */,
            0 /* down_time */,
            0 /* event_time */);
    }
    void produce_a_motion_event(float x, float y, nsecs_t t)
    {
        droidinput::PointerProperties filler_pointer_properties;
        droidinput::PointerCoords filler_pointer_coordinates;

        memset(&filler_pointer_properties, 0, sizeof(droidinput::PointerProperties));
        memset(&filler_pointer_coordinates, 0, sizeof(droidinput::PointerCoords));

        filler_pointer_coordinates.setAxisValue(AMOTION_EVENT_AXIS_X, x);
        filler_pointer_coordinates.setAxisValue(AMOTION_EVENT_AXIS_Y, y);

        input_publisher->publishMotionEvent(
            incrementing_seq_id,
            filler_device_id,
            0 /* source */,
            motion_event_action_flags,
            0 /* flags */,
            0 /* edge_flags */,
            0 /* meta_state */,
            0 /* button_state */,
            0 /* x_offset */, 0 /* y_offset */,
            0 /* x_precision */, 0 /* y_precision */,
            0 /* down_time */,
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
    static const int32_t motion_event_action_flags = mir_motion_action_move;
    // We have to have at least 1 pointer or the publisher will fail to marshal a motion event
    static const int32_t default_pointer_count = 1;
};

class AndroidInputReceiverSetup : public testing::Test
{
public:
    AndroidInputReceiverSetup()
    {
        auto status = droidinput::InputChannel::openInputFdPair(server_fd, client_fd);
        EXPECT_EQ(droidinput::OK, status);
    }
    ~AndroidInputReceiverSetup()
    {
        close(server_fd);
        close(client_fd);
    }

    void flush_channels()
    {
        fsync(server_fd);
        fsync(client_fd);
    }

    int server_fd, client_fd;

    static std::chrono::milliseconds const next_event_timeout;
};

std::chrono::milliseconds const AndroidInputReceiverSetup::next_event_timeout(1000);

}

TEST_F(AndroidInputReceiverSetup, receiever_takes_channel_fd)
{
    mircva::InputReceiver receiver(client_fd, std::make_shared<mircv::NullInputReceiverReport>());

    EXPECT_EQ(client_fd, receiver.fd());
}

TEST_F(AndroidInputReceiverSetup, receiver_receives_key_events)
{
    mircva::InputReceiver receiver(client_fd, std::make_shared<mircv::NullInputReceiverReport>());
    TestingInputProducer producer(server_fd);

    producer.produce_a_key_event();

    flush_channels();

    MirEvent ev;
    EXPECT_EQ(true, receiver.next_event(next_event_timeout, ev));

    EXPECT_EQ(mir_event_type_key, ev.type);
    EXPECT_EQ(producer.testing_key_event_scan_code, ev.key.scan_code);
}

TEST_F(AndroidInputReceiverSetup, receiver_handles_events)
{
    mircva::InputReceiver receiver(client_fd, std::make_shared<mircv::NullInputReceiverReport>());
    TestingInputProducer producer(server_fd);

    producer.produce_a_key_event();
    flush_channels();

    MirEvent ev;
    EXPECT_EQ(true, receiver.next_event(next_event_timeout, ev));

    flush_channels();

    EXPECT_TRUE (producer.must_receive_handled_signal());
}

TEST_F(AndroidInputReceiverSetup, receiver_consumes_batched_motion_events)
{
    mircva::InputReceiver receiver(client_fd, std::make_shared<mircv::NullInputReceiverReport>());
    TestingInputProducer producer(server_fd);

    // Produce 3 motion events before client handles any.
    producer.produce_a_motion_event(0, 0, 0);
    producer.produce_a_motion_event(0, 0, 0);
    producer.produce_a_motion_event(0, 0, 0);

    flush_channels();

    MirEvent ev;
    // Handle all three events as a batched event
    EXPECT_TRUE(receiver.next_event(next_event_timeout, ev));
    // Now there should be no events
    EXPECT_FALSE(receiver.next_event(std::chrono::milliseconds(1), ev)); // Minimal timeout needed for valgrind
}

TEST_F(AndroidInputReceiverSetup, slow_raw_input_doesnt_cause_frameskipping)
{   // Regression test for LP: #1372300
    using namespace testing;
    using namespace std::chrono;

    nsecs_t t = 0;

    mircva::InputReceiver receiver(
        client_fd, std::make_shared<mircv::NullInputReceiverReport>(),
        [&t](int) { return t; }
        );
    TestingInputProducer producer(server_fd);

    nsecs_t const one_millisecond = 1000000ULL;
    nsecs_t const one_second = 1000 * one_millisecond;
    nsecs_t const one_frame = one_second / 60;

    MirEvent ev;

    producer.produce_a_motion_event(123, 456, t);
    producer.produce_a_key_event();
    flush_channels();

    std::chrono::milliseconds const max_timeout(1000);

    // Key events don't get resampled. Will be reported first.
    ASSERT_TRUE(receiver.next_event(max_timeout, ev));
    ASSERT_EQ(mir_event_type_key, ev.type);

    // The motion is still too new. Won't be reported yet, but is batched.
    auto start = high_resolution_clock::now();
    ASSERT_FALSE(receiver.next_event(max_timeout, ev));
    auto end = high_resolution_clock::now();
    auto duration = end - start;

    // Verify we timed out in under a frame (LP: #1372300)
    // Sadly using real time as droidinput::Looper doesn't use a mocked clock.
    ASSERT_LT(duration_cast<nanoseconds>(duration).count(), one_frame);

    // Verify we don't use all the CPU by not sleeping (LP: #1373809)
    EXPECT_GT(duration_cast<nanoseconds>(duration).count(), one_millisecond);

    // But later in a frame or so, the motion will be reported:
    t += 2 * one_frame;  // Account for the new slower 55Hz event rate
    ASSERT_TRUE(receiver.next_event(max_timeout, ev));
    ASSERT_EQ(mir_event_type_motion, ev.type);
}

TEST_F(AndroidInputReceiverSetup, rendering_does_not_lag_behind_input)
{
    using namespace testing;

    nsecs_t t = 0;

    mircva::InputReceiver receiver(
        client_fd, std::make_shared<mircv::NullInputReceiverReport>(),
        [&t](int) { return t; }
        );
    TestingInputProducer producer(server_fd);

    nsecs_t const one_millisecond = 1000000ULL;
    nsecs_t const one_second = 1000 * one_millisecond;
    nsecs_t const device_sample_interval = one_second / 250;
    nsecs_t const frame_interval = one_second / 60;
    nsecs_t const gesture_duration = 1 * one_second;

    nsecs_t last_produced = 0;
    int frames_triggered = 0;

    for (t = 0; t < gesture_duration; t += one_millisecond)
    {
        if (!t || t >= (last_produced + device_sample_interval))
        {
            last_produced = t;
            float a = t * M_PI / 1000000.0f;
            float x = 500.0f * sinf(a);
            float y = 1000.0f * cosf(a);
            producer.produce_a_motion_event(x, y, t);
            flush_channels();
        }

        MirEvent ev;
        if (receiver.next_event(std::chrono::milliseconds(0), ev))
            ++frames_triggered;
    }

    // If the rendering time resulting from the gesture is longer than the
    // gesture itself then that's laggy...
    nsecs_t render_duration = frame_interval * frames_triggered;
    EXPECT_THAT(render_duration, Le(gesture_duration));

    int average_lag_milliseconds = (render_duration - gesture_duration) /
                                   (frames_triggered * one_millisecond);
    EXPECT_THAT(average_lag_milliseconds, Le(1));
}

TEST_F(AndroidInputReceiverSetup, input_comes_in_phase_with_rendering)
{
    using namespace testing;

    nsecs_t t = 0;

    mircva::InputReceiver receiver(
        client_fd, std::make_shared<mircv::NullInputReceiverReport>(),
        [&t](int) { return t; }
        );
    TestingInputProducer producer(server_fd);

    nsecs_t const one_millisecond = 1000000ULL;
    nsecs_t const one_second = 1000 * one_millisecond;
    nsecs_t const device_sample_interval = one_second / 125;
    nsecs_t const frame_interval = one_second / 60;
    nsecs_t const gesture_duration = 10 * one_second;

    nsecs_t last_produced = 0, last_consumed = 0, last_rendered = 0;
    nsecs_t last_in_phase = 0;

    for (t = 0; t < gesture_duration; t += one_millisecond)
    {
        if (!t || t >= (last_produced + device_sample_interval))
        {
            last_produced = t;
            float a = t * M_PI / 1000000.0f;
            float x = 500.0f * sinf(a);
            float y = 1000.0f * cosf(a);
            producer.produce_a_motion_event(x, y, t);
            flush_channels();
        }

        MirEvent ev;
        if (receiver.next_event(std::chrono::milliseconds(0), ev))
            last_consumed = t;

        if (t >= (last_rendered + frame_interval))
        {
            last_rendered = t;

            if (last_consumed)
            {
                auto lag = last_rendered - last_consumed;

                // How often does vsync drift in phase (very close) with the
                // time that we emitted/consumed input events?
                if (lag < 4*one_millisecond)
                    last_in_phase = t;
                last_consumed = 0;
            }
        }

        // Verify input and vsync come into phase at least a few times every
        // second (if not always). This ensure visible lag is minimized.
        ASSERT_GE(250*one_millisecond, (t - last_in_phase));
    }
}

