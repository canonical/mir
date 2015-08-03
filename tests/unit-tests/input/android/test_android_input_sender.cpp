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
#include "mir/events/event_builders.h"

#include "src/server/input/android/android_input_channel.h"
#include "src/server/input/android/input_sender.h"
#include "src/server/report/null_report_factory.h"

#include "mir/test/doubles/stub_scene_surface.h"
#include "mir/test/doubles/mock_input_surface.h"
#include "mir/test/doubles/mock_input_send_observer.h"
#include "mir/test/doubles/stub_scene.h"
#include "mir/test/doubles/mock_scene.h"
#include "mir/test/doubles/triggered_main_loop.h"
#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"

#include "androidfw/Input.h"
#include "androidfw/InputTransport.h"

#include "mir/input/input_report.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/exception/all.hpp>

//#include <algorithm>
#include <cstring>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace mia = mi::android;
namespace mev = mir::events;
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
    MOCK_METHOD2(received_event_finished_signal, void(int dest_fd, uint32_t seq_id));
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
    float test_x_coord[2] = {12, 23};
    float test_y_coord[2] = {17, 9};
    
    AndroidInputSender()
       : key_event(mev::make_event(MirInputDeviceId(), std::chrono::nanoseconds(1), mir_keyboard_action_down,
                                    7, test_scan_code, mir_input_event_modifier_none)),
          motion_event(mev::make_event(MirInputDeviceId(), std::chrono::nanoseconds(-1), mir_input_event_modifier_none))
    {
        using namespace ::testing;

        mev::add_touch(*motion_event, 0, mir_touch_action_change, mir_touch_tooltype_finger, test_x_coord[0], test_y_coord[0],
                       24, 25, 26, 27);
        mev::add_touch(*motion_event, 1, mir_touch_action_change, mir_touch_tooltype_finger, test_x_coord[1], test_y_coord[1],
                       24, 25, 26, 27);

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

    std::shared_ptr<mi::InputChannel> channel = std::make_shared<mia::AndroidInputChannel>();
    mtd::StubSceneSurface stub_surface{channel->server_fd()};
    droidinput::sp<droidinput::InputChannel> client_channel{new droidinput::InputChannel(droidinput::String8("test"), channel->client_fd())};
    droidinput::InputConsumer consumer{client_channel};

    mtd::TriggeredMainLoop loop;
    testing::NiceMock<mtd::MockInputSendObserver> observer;
    testing::NiceMock<MockInputReport> mock_input_report;

    mir::EventUPtr key_event;
    mir::EventUPtr motion_event;

    droidinput::MotionEvent client_motion_event;
    droidinput::KeyEvent client_key_event;

    testing::NiceMock<MockInputEventFactory> event_factory;

    droidinput::InputEvent * event= nullptr;
    uint32_t seq = 0;

    FakeScene fake_scene;
    mia::InputSender sender{mt::fake_shared(fake_scene), mt::fake_shared(loop), mt::fake_shared(observer), mt::fake_shared(mock_input_report)};
};

TEST_F(AndroidInputSender, subscribes_to_scene)
{
    using namespace ::testing;
    NiceMock<mtd::MockScene> mock_scene;

    EXPECT_CALL(mock_scene, add_observer(_));
    mia::InputSender sender(mt::fake_shared(mock_scene), mt::fake_shared(loop), mt::fake_shared(observer), mr::null_input_report());
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
    }
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
    EXPECT_CALL(observer,
                send_suceeded(mt::MirTouchEventMatches(*motion_event),
                              &stub_surface,
                              mi::InputSendObserver::consumed));

    consumer.sendFinishedSignal(seq, true);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, reports_receival_of_finish_signal)
{
    register_surface();

    sender.send_event(*motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event));
    EXPECT_CALL(mock_input_report, received_event_finished_signal(channel->server_fd(), seq));

    consumer.sendFinishedSignal(seq, true);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, finish_signal_triggers_success_callback_as_not_consumed)
{
    register_surface();

    sender.send_event(*motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &seq, &event));
    EXPECT_CALL(observer,
                send_suceeded(mt::MirTouchEventMatches(*motion_event),
                              &stub_surface,
                              mi::InputSendObserver::not_consumed));

    consumer.sendFinishedSignal(seq, false);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, unordered_finish_signal_triggers_the_right_callback)
{
    register_surface();

    auto another_key_event = mev::make_event(MirInputDeviceId(), std::chrono::nanoseconds(1), mir_keyboard_action_down,
                                             9, test_scan_code, mir_input_event_modifier_none);

    sender.send_event(*key_event, channel);
    sender.send_event(*another_key_event, channel);

    uint32_t first_sequence, second_sequence;
    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &first_sequence, &event));
    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, std::chrono::nanoseconds(-1), &second_sequence, &event));

    EXPECT_CALL(observer,
                send_suceeded(mt::MirKeyEventMatches(*another_key_event),
                              &stub_surface,
                              mi::InputSendObserver::consumed));
    EXPECT_CALL(observer,
                send_suceeded(mt::MirKeyEventMatches(*key_event),
                              &stub_surface,
                              mi::InputSendObserver::not_consumed));
    
    consumer.sendFinishedSignal(second_sequence, true);
    consumer.sendFinishedSignal(first_sequence, false);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, observer_notified_on_disapeared_surface )
{
    register_surface();

    sender.send_event(*key_event, channel);
    EXPECT_CALL(
        observer,
        send_failed(mt::MirKeyEventMatches(*key_event), &stub_surface, mir::input::InputSendObserver::surface_disappeared));
    deregister_surface();
}

TEST_F(AndroidInputSender, alarm_created_for_input_send)
{
    using namespace ::testing;

    register_surface();

    EXPECT_CALL(loop, create_alarm(An<std::function<void()> const&>()));
    sender.send_event(*key_event, channel);
}

TEST_F(AndroidInputSender, observer_informed_on_response_timeout)
{
    register_surface();

    sender.send_event(*key_event, channel);
    EXPECT_CALL(
        observer,
        send_failed(mt::MirKeyEventMatches(*key_event), &stub_surface, mir::input::InputSendObserver::no_response_received));

    loop.fire_all_alarms();
}

TEST_F(AndroidInputSender, observer_informed_about_closed_socket_on_send_event)
{
    register_surface();

    EXPECT_CALL(
        observer,
        send_failed(mt::MirKeyEventMatches(*key_event), &stub_surface, mir::input::InputSendObserver::socket_error));
    ::close(channel->client_fd());
    sender.send_event(*key_event, channel);
}
