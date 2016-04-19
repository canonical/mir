/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#include "mir/events/event_private.h"

#include "src/server/input/channel.h"
#include "src/server/input/android/input_sender.h"
#include "src/server/input/default_event_builder.h"
#include "src/server/report/null_report_factory.h"

#include "mir/test/doubles/stub_scene_surface.h"
#include "mir/test/doubles/mock_input_surface.h"
#include "mir/test/doubles/stub_scene.h"
#include "mir/test/doubles/mock_scene.h"
#include "mir/test/doubles/triggered_main_loop.h"
#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"


#include "androidfw/Input.h"
#include "androidfw/InputTransport.h"

#include "mir/input/input_report.h"
#include "mir/cookie/authority.h"
#include "mir/cookie/blob.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/exception/all.hpp>

#include <cstring>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mr = mir::report;
namespace mtd = mt::doubles;
namespace droidinput = android;

using testing::_;

namespace
{

class MockInputEventFactory : public droidinput::InputEventFactoryInterface
{
public:
    MOCK_METHOD0(createKeyEvent, droidinput::KeyEvent*());
    MOCK_METHOD0(createMotionEvent, droidinput::MotionEvent*());
};

class MockInputReport : public mir::input::InputReport
{
public:
    MOCK_METHOD4(received_event_from_kernel, void(int64_t when, int type, int code, int value));
    MOCK_METHOD3(published_key_event, void(int dest_fd, uint32_t seq_id, int64_t event_time));
    MOCK_METHOD3(published_motion_event, void(int dest_fd, uint32_t seq_id, int64_t event_time));
    MOCK_METHOD2(opened_input_device, void(char const* device_name, char const* input_platform));
    MOCK_METHOD2(failed_to_open_input_device, void(char const* device_name, char const* input_platform));
};

class FakeScene : public mtd::StubScene
{
public:
    void add_observer(std::shared_ptr<ms::Observer> const& observer) override
    {
        this->observer = observer;
    }
    void remove_observer(std::weak_ptr<ms::Observer> const&) override
    {
        observer.reset();
    }

    std::shared_ptr<ms::Observer> observer;
};
}

class AndroidInputSender : public ::testing::Test
{
public:
    int const test_scan_code = 32;
    size_t const test_pointer_count = 2;
    float const test_x_coord[2] = {12, 23};
    float const test_y_coord[2] = {17, 9};
    MirTouchTooltype const tool = mir_touch_tooltype_finger;
    float const pressure = 0.6f;
    float const major = 8;
    float const minor = 4;
    float const size = 8;
    mir::geometry::Displacement const movement{10, -10};
    std::shared_ptr<mir::cookie::Authority> const cookie_authority;
    mi::DefaultEventBuilder builder;

    AndroidInputSender()
        : cookie_authority(mir::cookie::Authority::create_from({0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88})),
          builder(MirInputDeviceId(), cookie_authority),
          key_event(builder.key_event(std::chrono::nanoseconds(1), mir_keyboard_action_down, 7, test_scan_code)),
          motion_event(builder.touch_event(std::chrono::nanoseconds(-1))),
          pointer_event(builder.pointer_event(std::chrono::nanoseconds(123), mir_pointer_action_motion,
                                        mir_pointer_button_primary, 0.0f, 0.0f,
                                        movement.dx.as_float(), movement.dy.as_float()))

    {
        using namespace ::testing;

        builder.add_touch(*motion_event, 0, mir_touch_action_change, tool, test_x_coord[0], test_y_coord[0],
                       pressure, major, minor, size);
        builder.add_touch(*motion_event, 1, mir_touch_action_change, tool, test_x_coord[1], test_y_coord[1],
                       pressure, major, minor, size);

        ON_CALL(event_factory, createKeyEvent()).WillByDefault(Return(&client_key_event));
        ON_CALL(event_factory, createMotionEvent()).WillByDefault(Return(&client_motion_event));
    }

    void register_surface()
    {
        fake_scene.observer->surface_added(&stub_surface);
    }

    void deregister_surface()
    {
        fake_scene.observer->surface_removed(&stub_surface);
    }

    std::shared_ptr<mi::InputChannel> channel = std::make_shared<mi::Channel>();
    mtd::StubSceneSurface stub_surface{channel->server_fd()};
    droidinput::sp<droidinput::InputChannel> client_channel{new droidinput::InputChannel(droidinput::String8("test"), channel->client_fd())};
    droidinput::InputConsumer consumer{client_channel};

    mtd::TriggeredMainLoop loop;
    testing::NiceMock<MockInputReport> mock_input_report;

    mir::EventUPtr key_event;
    mir::EventUPtr motion_event;
    mir::EventUPtr pointer_event;

    droidinput::MotionEvent client_motion_event;
    droidinput::KeyEvent client_key_event;

    testing::NiceMock<MockInputEventFactory> event_factory;

    droidinput::InputEvent * event= nullptr;
    uint32_t seq = 0;

    FakeScene fake_scene;
    mia::InputSender sender{mt::fake_shared(fake_scene), mt::fake_shared(loop), mt::fake_shared(mock_input_report)};
};

TEST_F(AndroidInputSender, subscribes_to_scene)
{
    using namespace ::testing;
    NiceMock<mtd::MockScene> mock_scene;

    EXPECT_CALL(mock_scene, add_observer(_));
    mia::InputSender sender(mt::fake_shared(mock_scene), mt::fake_shared(loop), mr::null_input_report());
}

TEST_F(AndroidInputSender, throws_on_unknown_channel)
{
    EXPECT_THROW(sender.send_event(*key_event, channel), boost::exception);
}

TEST_F(AndroidInputSender,throws_on_deregistered_channels)
{
    register_surface();
    deregister_surface();

    EXPECT_THROW(sender.send_event(*key_event, channel), boost::exception);
}

TEST_F(AndroidInputSender, first_send_on_surface_registers_server_fd)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, register_fd_handler(ElementsAre(channel->server_fd()),_,_));

    sender.send_event(*key_event, channel);
}

TEST_F(AndroidInputSender, second_send_on_surface_does_not_register_server_fd)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, register_fd_handler(ElementsAre(channel->server_fd()),_,_)).Times(1);

    sender.send_event(*key_event, channel);
    sender.send_event(*key_event, channel);
}

TEST_F(AndroidInputSender, removal_of_surface_after_send_unregisters_server_fd)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, unregister_fd_handler(_)).Times(1);

    sender.send_event(*key_event, channel);
    sender.send_event(*key_event, channel);
    deregister_surface();

    Mock::VerifyAndClearExpectations(&loop);
}

TEST_F(AndroidInputSender, can_send_consumeable_mir_key_events)
{
    register_surface();

    sender.send_event(*key_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event));

    EXPECT_EQ(&client_key_event, event);
    EXPECT_EQ(test_scan_code, client_key_event.getScanCode());
}

TEST_F(AndroidInputSender, reports_published_key_events)
{
    register_surface();
    auto expected_time = mir_input_event_get_event_time(mir_event_get_input_event(key_event.get()));

    EXPECT_CALL(mock_input_report, published_key_event(channel->server_fd(),_,expected_time));
    sender.send_event(*key_event, channel);
}


TEST_F(AndroidInputSender, reports_published_motion_events)
{
    register_surface();
    auto expected_time = mir_input_event_get_event_time(mir_event_get_input_event(motion_event.get()));

    EXPECT_CALL(mock_input_report, published_motion_event(channel->server_fd(),_,expected_time));
    sender.send_event(*motion_event, channel);
}

TEST_F(AndroidInputSender, can_send_consumeable_mir_motion_events)
{
    using namespace ::testing;
    register_surface();

    sender.send_event(*motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event));

    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(test_pointer_count, client_motion_event.getPointerCount());
    EXPECT_EQ(0, client_motion_event.getXOffset());
    EXPECT_EQ(0, client_motion_event.getYOffset());

    for (size_t i = 0; i != test_pointer_count; ++i)
    {
        EXPECT_EQ(test_x_coord[i], client_motion_event.getRawX(i)) << "When i=" << i;
        EXPECT_EQ(test_x_coord[i], client_motion_event.getX(i)) << "When i=" << i;
        EXPECT_EQ(test_y_coord[i], client_motion_event.getRawY(i)) << "When i=" << i;
        EXPECT_EQ(test_y_coord[i], client_motion_event.getY(i)) << "When i=" << i;
        EXPECT_EQ(AMOTION_EVENT_TOOL_TYPE_FINGER, client_motion_event.getToolType(i)) << "When i=" << i;
    }
    EXPECT_EQ(AINPUT_SOURCE_TOUCHSCREEN, client_motion_event.getSource());
}

TEST_F(AndroidInputSender, non_input_clients_dont_crash_the_server)
{   // Regression test for LP: #1528438
    register_surface();

    EXPECT_NO_THROW(
        {
            for (int i = 0; i < 10000; ++i)
                sender.send_event(*motion_event, channel);
        });
}

TEST_F(AndroidInputSender, sends_pointer_events)
{
    using namespace ::testing;
    register_surface();

    sender.send_event(*pointer_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event));

    EXPECT_EQ(1, client_motion_event.getPointerCount());

    EXPECT_EQ(movement.dx.as_float(), client_motion_event.getAxisValue(AMOTION_EVENT_AXIS_RX, 0));
    EXPECT_EQ(movement.dy.as_float(), client_motion_event.getAxisValue(AMOTION_EVENT_AXIS_RY, 0));
    EXPECT_EQ(AMOTION_EVENT_TOOL_TYPE_MOUSE, client_motion_event.getToolType(0));
    EXPECT_EQ(AINPUT_SOURCE_MOUSE, client_motion_event.getSource());
}

TEST_F(AndroidInputSender, response_keeps_fd_registered)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, unregister_fd_handler(_)).Times(0);

    sender.send_event(*key_event, channel);
    consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event);
    consumer.sendFinishedSignal(seq, true);
    loop.trigger_pending_fds();

    Mock::VerifyAndClearExpectations(&loop);
}

TEST_F(AndroidInputSender, finish_signal_triggers_success_callback_as_consumed)
{
    register_surface();

    sender.send_event(*motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event));

    consumer.sendFinishedSignal(seq, true);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, finish_signal_triggers_success_callback_as_not_consumed)
{
    register_surface();

    sender.send_event(*motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event));

    consumer.sendFinishedSignal(seq, false);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, encodes_multiple_touch_contact_downs_as_multiple_events)
{
    // TODO When the rework of the transport encoding is finished we might want to revisit this decision
    register_surface();

    auto touch_ev_with_simultaneous_down = builder.touch_event(std::chrono::nanoseconds(86));
    builder.add_touch(*touch_ev_with_simultaneous_down, 2, mir_touch_action_down, tool, 50, 60, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 4, mir_touch_action_down, tool, 80, 90, pressure, major, minor, size);

    sender.send_event(*touch_ev_with_simultaneous_down, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_DOWN, client_motion_event.getActionMasked());
    EXPECT_EQ(0, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(1, client_motion_event.getPointerCount());

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_DOWN, client_motion_event.getActionMasked());
    EXPECT_EQ(1, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(4, client_motion_event.getPointerId(1));
    EXPECT_EQ(2, client_motion_event.getPointerCount());
}

TEST_F(AndroidInputSender, encodes_multiple_touch_contact_downs_as_multiple_events_but_keeps_only_moved_contacts_in_place)
{
    // TODO When the rework of the transport encoding is finished we might want to revisit this decision
    register_surface();

    auto touch_ev_with_simultaneous_down = builder.touch_event(std::chrono::nanoseconds(86));
    builder.add_touch(*touch_ev_with_simultaneous_down, 2, mir_touch_action_down, tool, 50, 60, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 3, mir_touch_action_change, tool, 10, 10, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 4, mir_touch_action_down, tool, 80, 90, pressure, major, minor, size);

    sender.send_event(*touch_ev_with_simultaneous_down, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_DOWN, client_motion_event.getActionMasked());
    EXPECT_EQ(0, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(3, client_motion_event.getPointerId(1));
    EXPECT_EQ(2, client_motion_event.getPointerCount());

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_DOWN, client_motion_event.getActionMasked());
    EXPECT_EQ(2, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(3, client_motion_event.getPointerId(1));
    EXPECT_EQ(4, client_motion_event.getPointerId(2));
    EXPECT_EQ(3, client_motion_event.getPointerCount());
}

TEST_F(AndroidInputSender, encodes_multiple_releases_in_several_events)
{
    // TODO When the rework of the transport encoding is finished we might want to revisit this decision
    register_surface();

    auto touch_ev_with_simultaneous_down = builder.touch_event(std::chrono::nanoseconds(86));
    builder.add_touch(*touch_ev_with_simultaneous_down, 2, mir_touch_action_up, tool, 50, 60, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 3, mir_touch_action_change, tool, 10, 10, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 4, mir_touch_action_up, tool, 80, 90, pressure, major, minor, size);

    sender.send_event(*touch_ev_with_simultaneous_down, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_UP, client_motion_event.getActionMasked());
    EXPECT_EQ(0, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(3, client_motion_event.getPointerId(1));
    EXPECT_EQ(4, client_motion_event.getPointerId(2));
    EXPECT_EQ(3, client_motion_event.getPointerCount());

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_UP, client_motion_event.getActionMasked());
    EXPECT_EQ(1, client_motion_event.getActionIndex());
    EXPECT_EQ(3, client_motion_event.getPointerId(0));
    EXPECT_EQ(4, client_motion_event.getPointerId(1));
    EXPECT_EQ(2, client_motion_event.getPointerCount());
}

TEST_F(AndroidInputSender, encodes_combinations_of_up_and_down_one_by_one)
{
    // TODO When the rework of the transport encoding is finished we might want to revisit this decision
    register_surface();

    auto touch_ev_with_simultaneous_down = builder.touch_event(std::chrono::nanoseconds(86));
    builder.add_touch(*touch_ev_with_simultaneous_down, 2, mir_touch_action_up, tool, 50, 60, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 3, mir_touch_action_change, tool, 10, 10, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 4, mir_touch_action_down, tool, 80, 90, pressure, major, minor, size);

    sender.send_event(*touch_ev_with_simultaneous_down, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_UP, client_motion_event.getActionMasked());
    EXPECT_EQ(0, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(3, client_motion_event.getPointerId(1));
    EXPECT_EQ(2, client_motion_event.getPointerCount());

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_DOWN, client_motion_event.getActionMasked());
    EXPECT_EQ(1, client_motion_event.getActionIndex());
    EXPECT_EQ(3, client_motion_event.getPointerId(0));
    EXPECT_EQ(4, client_motion_event.getPointerId(1));
    EXPECT_EQ(2, client_motion_event.getPointerCount());
}

TEST_F(AndroidInputSender, encodes_combinations_of_down_and_up_one_by_one)
{
    // TODO When the rework of the transport encoding is finished we might want to revisit this decision
    register_surface();

    auto touch_ev_with_simultaneous_down = builder.touch_event(std::chrono::nanoseconds(86));
    builder.add_touch(*touch_ev_with_simultaneous_down, 2, mir_touch_action_down, tool, 50, 60, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 3, mir_touch_action_change, tool, 10, 10, pressure, major, minor, size);
    builder.add_touch(*touch_ev_with_simultaneous_down, 4, mir_touch_action_up, tool, 80, 90, pressure, major, minor, size);

    sender.send_event(*touch_ev_with_simultaneous_down, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_DOWN, client_motion_event.getActionMasked());
    EXPECT_EQ(0, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(3, client_motion_event.getPointerId(1));
    EXPECT_EQ(4, client_motion_event.getPointerId(2));
    EXPECT_EQ(3, client_motion_event.getPointerCount());

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(86), &seq, &event));
    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(AMOTION_EVENT_ACTION_UP, client_motion_event.getActionMasked());
    EXPECT_EQ(2, client_motion_event.getActionIndex());
    EXPECT_EQ(2, client_motion_event.getPointerId(0));
    EXPECT_EQ(3, client_motion_event.getPointerId(1));
    EXPECT_EQ(4, client_motion_event.getPointerId(2));
    EXPECT_EQ(3, client_motion_event.getPointerCount());
}
