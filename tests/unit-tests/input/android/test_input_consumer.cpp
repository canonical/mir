/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "androidfw/Input.h"
#include "androidfw/InputTransport.h"
#include "src/server/input/channel.h"
#include "mir/input/android/event_conversion_helpers.h"
#include "mir/input/android/android_input_lexicon.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/point.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <vector>
#include <cstring>

using namespace testing;
using namespace std::literals::chrono_literals;
namespace mia = mir::input::android;
namespace mi = mir::input;
namespace geom = mir::geometry;

namespace
{
struct EventFactory : android::InputEventFactoryInterface
{
    android::KeyEvent key;
    android::MotionEvent motion;
    android::KeyEvent* createKeyEvent()
    {
        return &key;
    }
    android::MotionEvent* createMotionEvent()
    {
        return &motion;
    }
};
}

struct InputConsumerTest : ::testing::Test
{
    MOCK_METHOD3(pointer_movement,void(geom::Point pos, geom::Displacement movement, geom::Displacement scroll));
    uint32_t seq{0};
    mir::cookie::Blob default_cookie;
    EventFactory events;
    std::chrono::milliseconds current_frame_time = 0ms;
    mi::Channel channel;
    geom::Displacement no_move{0,0};
    geom::Displacement no_scroll{0,0};
    geom::Point origin{0,0};
    droidinput::sp<droidinput::InputChannel> client_channel =
        new droidinput::InputChannel("test_client", channel.client_fd());
    droidinput::sp<droidinput::InputChannel> server_channel =
        new droidinput::InputChannel("test_server", channel.server_fd());

    droidinput::InputPublisher publisher{server_channel};
    droidinput::InputConsumer consumer{client_channel};

    InputConsumerTest()
    {
        std::memset(&default_cookie, 0, sizeof default_cookie);
    }

    const MirInputDeviceId touchscreen_device = 2;
    const MirInputDeviceId mouse_device = 3;
    struct TouchEvent
    {
        int action; // only one action per event - android InputTransport restriction
        std::vector<geom::Point> positions;
        std::chrono::nanoseconds tp;
    };
    void send_touch_event(TouchEvent const& event)
    {
        std::vector<droidinput::PointerCoords> coords(event.positions.size());
        std::vector<droidinput::PointerProperties> properties(event.positions.size());
        std::memset(coords.data(), 0, sizeof coords[0] * coords.size());
        std::memset(properties.data(), 0, sizeof properties[0] * properties.size());
        auto const x_offset = 0.0f;
        auto const y_offset = 0.0f;
        auto const x_precision = 0;
        auto const y_precision = 0;
        auto const flags = 0;
        auto const edge_flags = 0;
        auto const button_state = 0;
        int contacts_in_event = 0;
        for (auto const& contact : event.positions)
        {
            coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_X, contact.x.as_int());
            coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_Y, contact.y.as_int());
            coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, 5);
            coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, 5);
            coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_SIZE, 5);
            coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, 0.7);
            properties[contacts_in_event].toolType = AMOTION_EVENT_TOOL_TYPE_FINGER;
            properties[contacts_in_event].id = contacts_in_event;
        }

        publisher.publishMotionEvent(++seq, touchscreen_device, AINPUT_SOURCE_TOUCHSCREEN, event.action, flags,
                                     edge_flags, 0, button_state, x_offset, y_offset, x_precision, y_precision,
                                     default_cookie, event.tp, event.tp, contacts_in_event, properties.data(),
                                     coords.data());
    }

    struct PointerEvent
    {
        MirPointerAction action;
        MirPointerButtons buttons;
        geom::Point position;
        geom::Displacement movement;
        geom::Displacement scroll;
        std::chrono::nanoseconds tp;
    };
    void send_pointer_event(PointerEvent const& event)
    {
        droidinput::PointerCoords pointer_coord;
        droidinput::PointerProperties pointer_properties;
        std::memset(&pointer_coord, 0, sizeof(pointer_coord));
        std::memset(&pointer_properties, 0, sizeof(pointer_properties));

        pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_X, event.position.x.as_int());
        pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_Y, event.position.y.as_int());
        pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_HSCROLL, event.scroll.dx.as_int());
        pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_VSCROLL, event.scroll.dy.as_int());
        pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_RX, event.movement.dx.as_int());
        pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_RY, event.movement.dy.as_int());
        pointer_properties.toolType = AMOTION_EVENT_TOOL_TYPE_MOUSE;
        pointer_properties.id = 0;

        auto const x_offset = 0.0f;
        auto const y_offset = 0.0f;
        auto const x_precision = 0;
        auto const y_precision = 0;
        auto const flags = 0;
        auto const buttons = 0;
        auto const modifiers = 0;
        auto const edge_flags = 0;
        publisher.publishMotionEvent(++seq, mouse_device, AINPUT_SOURCE_MOUSE,
                                     mia::android_pointer_action_from_mir(event.action, event.buttons), flags,
                                     edge_flags, modifiers, buttons, x_offset, y_offset, x_precision, y_precision,
                                     default_cookie, event.tp, event.tp, 1, &pointer_properties, &pointer_coord);
    }

    void handle_event(droidinput::InputEvent* event)
    {
        if (event->getType() == AINPUT_EVENT_TYPE_KEY)
        {
        }
        else
        {
            if (mia::android_source_id_is_pointer_device(event->getSource()))
            {
                auto mev = static_cast<const droidinput::MotionEvent*>(event);
                pointer_movement(
                    {mev->getX(0), mev->getY(0)},
                    {mev->getRawAxisValue(AMOTION_EVENT_AXIS_RX, 0),
                     mev->getRawAxisValue(AMOTION_EVENT_AXIS_RY, 0)},
                    {mev->getRawAxisValue(AMOTION_EVENT_AXIS_HSCROLL, 0),
                     mev->getRawAxisValue(AMOTION_EVENT_AXIS_VSCROLL, 0)});
            }
        }
    }
    void receive_events()
    {
        android::InputEvent *received_event = nullptr;
        uint32_t seq_id;
        do
        {
            auto result = consumer.consume(&events, true, current_frame_time, &seq_id, &received_event);

            if (result == droidinput::OK)
                handle_event(received_event);

            if (consumer.hasPendingBatch())
                current_frame_time += fake_update_rate;
        } while(consumer.hasPendingBatch() || consumer.hasDeferredEvent());
    }

    std::chrono::milliseconds fake_update_rate = 1ms;
    void advance_frame_time_to(std::chrono::milliseconds time)
    {
        current_frame_time = time;
    }

};

TEST_F(InputConsumerTest, emits_single_move_event_on_old_pointer_messages)
{
    EXPECT_CALL(*this, pointer_movement(geom::Point{4,11}, geom::Displacement{2,1}, no_scroll));

    send_pointer_event({mir_pointer_action_motion, 0, {2.0, 10.0}, {0.0, 0.0}, no_scroll, 0ns});
    send_pointer_event({mir_pointer_action_motion, 0, {3.0, 11.0}, {1.0, 1.0}, no_scroll, 1ms});
    send_pointer_event({mir_pointer_action_motion, 0, {4.0, 11.0}, {1.0, 0.0}, no_scroll, 2ms});

    advance_frame_time_to(16ms);
    receive_events();
}

TEST_F(InputConsumerTest, emits_move_events_on_recent_messages)
{
    EXPECT_CALL(*this, pointer_movement(geom::Point{10, 5}, geom::Displacement{0, 0}, no_scroll));
    EXPECT_CALL(*this, pointer_movement(geom::Point{15, 8}, geom::Displacement{5, 3}, no_scroll));
    EXPECT_CALL(*this, pointer_movement(geom::Point{12,10}, geom::Displacement{-3, 2}, no_scroll));

    send_pointer_event({mir_pointer_action_motion, 0, {10.0, 5.0}, {0.0, 0.0}, no_scroll, 0ns});
    send_pointer_event({mir_pointer_action_motion, 0, {15.0, 8.0}, {5.0, 3.0}, no_scroll, 1ms});
    send_pointer_event({mir_pointer_action_motion, 0, {12.0, 10.0}, {-3.0,2.0}, no_scroll, 2ms});

    advance_frame_time_to(2ms);
    receive_events();
}


TEST_F(InputConsumerTest, batched_and_resampled_events_contain_correct_relative_displacement)
{
    Sequence seq;
    EXPECT_CALL(*this, pointer_movement(geom::Point{0, 0}, geom::Displacement{0, 0}, no_scroll));
    EXPECT_CALL(*this, pointer_movement(geom::Point{27, 20}, geom::Displacement{27, 20}, no_scroll));
    EXPECT_CALL(*this, pointer_movement(geom::Point{55, 62}, geom::Displacement{28, 42}, no_scroll));
    EXPECT_CALL(*this, pointer_movement(geom::Point{67, 71}, geom::Displacement{12, 9}, no_scroll));
    MirPointerButtons button_down = mir_pointer_button_primary;
    fake_update_rate = 16ms;

    send_pointer_event({mir_pointer_action_button_down, button_down, {0, 0}, {0, 0}, no_scroll, 0ns});
    send_pointer_event({mir_pointer_action_motion, button_down, {10, 5}, {10, 5}, no_scroll, 2ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {18, 12}, {8, 7}, no_scroll, 4ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {27, 20}, {9, 8}, no_scroll, 8ms});

    // 5 ms RESAMPLE_LATENCY
    receive_events();

    send_pointer_event({mir_pointer_action_motion, button_down, {39, 36}, {12, 16}, no_scroll, 12ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {43, 56}, {4, 20}, no_scroll, 16ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {51, 59}, {8, 3}, no_scroll, 20ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {55, 62}, {4, 3}, no_scroll, 24ms});

    // 16 ms later
    receive_events();
    send_pointer_event({mir_pointer_action_motion, button_down, {59, 65}, {4, 3}, no_scroll, 28ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {63, 68}, {4, 3}, no_scroll, 32ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {67, 71}, {4, 3}, no_scroll, 36ms});

    // 16 ms later
    receive_events();
}

TEST_F(InputConsumerTest, correct_relative_displacement_when_switching_in_and_out_of_resampling)
{
    Sequence seq;
    EXPECT_CALL(*this, pointer_movement(geom::Point{27, 20}, geom::Displacement{27, 20}, no_scroll));
    // emitted due to press:
    EXPECT_CALL(*this, pointer_movement(geom::Point{39, 36}, geom::Displacement{12, 16}, no_scroll));
    EXPECT_CALL(*this, pointer_movement(geom::Point{55, 62}, geom::Displacement{16, 26}, no_scroll));
    // emit due to release:
    EXPECT_CALL(*this, pointer_movement(geom::Point{59, 65}, geom::Displacement{4, 3}, no_scroll));
    EXPECT_CALL(*this, pointer_movement(geom::Point{70, 72}, geom::Displacement{11, 7}, no_scroll));
    MirPointerButtons button_down = mir_pointer_button_primary;
    fake_update_rate = 16ms;

    send_pointer_event({mir_pointer_action_motion, 0, {10, 5}, {10, 5}, no_scroll, 0ns});
    send_pointer_event({mir_pointer_action_motion, 0, {18, 12}, {8, 7}, no_scroll, 4ms});
    send_pointer_event({mir_pointer_action_motion, 0, {27, 20}, {9, 8}, no_scroll, 8ms});

    // 5ms RESAMPLE_LATENCY
    receive_events();

    send_pointer_event({mir_pointer_action_button_down, button_down, {39, 36}, {12, 16}, no_scroll, 12ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {43, 56}, {4, 20}, no_scroll, 16ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {51, 59}, {8, 3}, no_scroll, 20ms});
    send_pointer_event({mir_pointer_action_motion, button_down, {55, 62}, {4, 3}, no_scroll, 24ms});

    // update rate + 16 ms later
    receive_events();

    send_pointer_event({mir_pointer_action_button_up, 0, {59, 65}, {4, 3}, no_scroll, 28ms});
    // handle release immediately..
    receive_events();

    send_pointer_event({mir_pointer_action_motion, 0, {63, 68}, {4, 3}, no_scroll, 32ms});
    send_pointer_event({mir_pointer_action_motion, 0, {67, 71}, {4, 3}, no_scroll, 36ms});
    send_pointer_event({mir_pointer_action_motion, 0, {70, 72}, {3, 1}, no_scroll, 40ms});

    receive_events();
}


TEST_F(InputConsumerTest, emits_scroll_events_on_each_recent_scroll_messages)
{
    EXPECT_CALL(*this, pointer_movement(origin, no_move, geom::Displacement{3, 0}));
    EXPECT_CALL(*this, pointer_movement(origin, no_move, geom::Displacement{0, 5}));
    EXPECT_CALL(*this, pointer_movement(origin, no_move, geom::Displacement{2, 5}));

    send_pointer_event({mir_pointer_action_motion, 0, origin, no_move, {3.0, 0.0}, 0ns});
    send_pointer_event({mir_pointer_action_motion, 0, origin, no_move, {0.0, 5.0}, 1ms});
    send_pointer_event({mir_pointer_action_motion, 0, origin, no_move, {2.0, 5.0}, 2ms});

    advance_frame_time_to(2ms);
    receive_events();
}

TEST_F(InputConsumerTest, emits_accumulated_scroll_event_on_old_messages)
{
    EXPECT_CALL(*this, pointer_movement(origin, no_move, geom::Displacement{5, 10}));

    send_pointer_event({mir_pointer_action_motion, 0, origin, no_move, {3.0, 0.0}, 0ns});
    send_pointer_event({mir_pointer_action_motion, 0, origin, no_move, {0.0, 5.0}, 1ms});
    send_pointer_event({mir_pointer_action_motion, 0, origin, no_move, {2.0, 5.0}, 2ms});

    advance_frame_time_to(16ms);
    receive_events();
}
