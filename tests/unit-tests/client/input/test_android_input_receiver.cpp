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

#include "src/client/input/android_input_receiver.h"
#include "mir_toolkit/input/event.h"

#include <androidfw/InputTransport.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unistd.h>
#include <memory>

namespace mclia = mir::client::input::android;

namespace droidinput = android;

namespace
{

class TestingInputProducer
{
public:
    TestingInputProducer(droidinput::sp<droidinput::InputChannel> input_channel) :
        input_publisher(std::make_shared<droidinput::InputPublisher>(input_channel)),
        incrementing_seq_id(1), // Sequence id must be > 0 or publisher will reject
        testing_key_event_key_code(13)
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
            testing_key_event_key_code,
            0 /* scan_code */,
            0 /* meta_state */,
            0 /* repeat_count */,
            0 /* down_time */,
            0 /* event_time */);
    }
    void produce_a_motion_event()
    {
        droidinput::PointerProperties filler_pointer_properties;
        droidinput::PointerCoords filler_pointer_coordinates;

        memset(&filler_pointer_properties, 0, sizeof(droidinput::PointerProperties));
        memset(&filler_pointer_coordinates, 0, sizeof(droidinput::PointerCoords));

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
            0 /* event_time */,
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
    int32_t testing_key_event_key_code;
    
    // Some default values
    // device_id must be > 0 or input publisher will reject
    static const int32_t filler_device_id = 1;
    // AMOTION_EVENT_ACTION_MOVE is necessary to engage batching behavior
    static const int32_t motion_event_action_flags = AMOTION_EVENT_ACTION_MOVE;
    // We have to have at least 1 pointer or the publisher will fail to marshal a motion event
    static const int32_t default_pointer_count = 1;
};

class AndroidInputReceiverSetup : public testing::Test
{
public:
    AndroidInputReceiverSetup()
    {
        droidinput::String8 const testing_channel_name = droidinput::String8("Terrapin");
        auto status = droidinput::InputChannel::openInputChannelPair(testing_channel_name, 
                                                                     android_client_channel,
                                                                     android_server_channel);
        
        EXPECT_EQ(droidinput::OK, status);
    }
    
    void flush_channels()
    {
        fsync(android_server_channel->getFd());
        fsync(android_client_channel->getFd());
    }

    droidinput::sp<droidinput::InputChannel> android_client_channel;
    droidinput::sp<droidinput::InputChannel> android_server_channel;
    
    static std::chrono::milliseconds const next_event_timeout;
};

std::chrono::milliseconds const AndroidInputReceiverSetup::next_event_timeout(1000);

}

TEST_F(AndroidInputReceiverSetup, receiever_takes_channel_fd)
{
    mclia::InputReceiver receiver(android_client_channel);
    
    EXPECT_EQ(android_client_channel->getFd(), receiver.fd());
}

TEST_F(AndroidInputReceiverSetup, receiver_receives_key_events)
{
    mclia::InputReceiver receiver(android_client_channel);
    TestingInputProducer producer(android_server_channel);
    
    producer.produce_a_key_event();
    
    flush_channels();
    
    MirEvent ev;
    EXPECT_EQ(true, receiver.next_event(next_event_timeout, ev));
    EXPECT_EQ(MIR_INPUT_EVENT_TYPE_KEY, ev.type);
    EXPECT_EQ(producer.testing_key_event_key_code, ev.details.key.key_code);
}

TEST_F(AndroidInputReceiverSetup, receiver_handles_events)
{
    mclia::InputReceiver receiver(android_client_channel);
    TestingInputProducer producer(android_server_channel);
    
    producer.produce_a_key_event();
    flush_channels();
    
    MirEvent ev;
    EXPECT_EQ(true, receiver.next_event(next_event_timeout, ev));
    
    flush_channels();
    
    EXPECT_TRUE (producer.must_receive_handled_signal());
}

TEST_F(AndroidInputReceiverSetup, receiver_consumes_batched_motion_events)
{
    mclia::InputReceiver receiver(android_client_channel);
    TestingInputProducer producer(android_server_channel);
    
    // Produce 3 motion events before client handles any.
    producer.produce_a_motion_event();
    producer.produce_a_motion_event();
    producer.produce_a_motion_event();

    flush_channels();
    
    MirEvent ev;
    // Handle all three events as a batched event
    EXPECT_TRUE(receiver.next_event(next_event_timeout, ev));
    // Now there should be no events
    EXPECT_FALSE(receiver.next_event(std::chrono::milliseconds(1), ev)); // Minimal timeout needed for valgrind
}

