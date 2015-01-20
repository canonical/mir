/*
 * Copyright © 2014 Canonical Ltd.
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

#include "src/server/input/android/android_input_channel.h"
#include "src/server/input/android/input_sender.h"
#include "src/server/report/null_report_factory.h"

#include "mir_test_doubles/mock_main_loop.h"
#include "mir_test_doubles/stub_scene_surface.h"
#include "mir_test_doubles/mock_input_surface.h"
#include "mir_test_doubles/mock_input_send_observer.h"
#include "mir_test_doubles/stub_scene.h"
#include "mir_test_doubles/mock_scene.h"
#include "mir_test_doubles/stub_timer.h"
#include "mir_test/fake_shared.h"
#include "mir_test/event_matchers.h"

#include "androidfw/Input.h"
#include "androidfw/InputTransport.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/exception/all.hpp>

#include <vector>
#include <algorithm>
#include <cstring>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mr = mir::report;
namespace mtd = mt::doubles;
namespace droidinput = android;

namespace
{

class MockInputEventFactory : public droidinput::InputEventFactoryInterface
{
public:
    MOCK_METHOD0(createKeyEvent, droidinput::KeyEvent*());
    MOCK_METHOD0(createMotionEvent, droidinput::MotionEvent*());
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

class TriggeredMainLoop : public ::testing::NiceMock<mtd::MockMainLoop>
{
public:
    typedef ::testing::NiceMock<mtd::MockMainLoop> base;
    typedef std::function<void(int)> fd_callback;
    typedef std::function<void(int)> signal_callback;
    typedef std::function<void()> callback;

    void register_fd_handler(std::initializer_list<int> fds, void const* owner, fd_callback const& handler) override
    {
        base::register_fd_handler(fds, owner, handler);
        for (int fd : fds)
        {
            fd_callbacks.emplace_back(Item{fd, owner, handler});
        }
    }

    void unregister_fd_handler(void const* owner)
    {
        base::unregister_fd_handler(owner);
        fd_callbacks.erase(
            remove_if(
                begin(fd_callbacks),
                end(fd_callbacks),
                [owner](Item const& item)
                {
                    return item.owner == owner;
                }),
            end(fd_callbacks)
            );
    }

    void trigger_pending_fds()
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = 0;

        for (auto const & item : fd_callbacks)
        {
            FD_SET(item.fd, &read_fds);
            max_fd = std::max(item.fd, max_fd);
        }

        struct timeval do_not_wait{0, 0};

        if (select(max_fd+1, &read_fds, nullptr, nullptr, &do_not_wait))
        {
            for (auto const & item : fd_callbacks)
            {
                FD_ISSET(item.fd, &read_fds);
                item.callback(item.fd);
            }
        }
    }

    std::unique_ptr<mir::time::Alarm> notify_in(std::chrono::milliseconds delay,
                                                callback call) override
    {
        base::notify_in(delay, call);
        timeout_callbacks.push_back(call);
        return std::unique_ptr<mir::time::Alarm>{new mtd::StubAlarm};
    }

    void fire_all_alarms()
    {
        for(auto const& callback : timeout_callbacks)
            callback();
    }

private:
    std::vector<callback> timeout_callbacks;

private:
    struct Item
    {
        int fd;
        void const* owner;
        fd_callback callback;
    };
    std::vector<Item> fd_callbacks;
};

}

class AndroidInputSender : public ::testing::Test
{
public:
    AndroidInputSender()
    {
        using namespace ::testing;

        std::memset(&key_event, 0, sizeof key_event);
        std::memset(&motion_event, 0, sizeof motion_event);

        key_event.type = mir_event_type_key;
        key_event.key.scan_code = 32;
        key_event.key.action = mir_key_action_down;

        motion_event.type = mir_event_type_motion;
        motion_event.motion.pointer_count = 2;
        motion_event.motion.device_id = 3;
        motion_event.motion.pointer_coordinates[0].x = 12;
        motion_event.motion.pointer_coordinates[0].y = 23;
        motion_event.motion.pointer_coordinates[0].pressure = 50;
        motion_event.motion.pointer_coordinates[0].tool_type = mir_motion_tool_type_finger;
        motion_event.motion.pointer_coordinates[1].tool_type = mir_motion_tool_type_finger;
        motion_event.motion.pointer_coordinates[1].x = 55;
        motion_event.motion.pointer_coordinates[1].y = 42;
        motion_event.motion.pointer_coordinates[1].pressure = 50;

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

    TriggeredMainLoop loop;
    testing::NiceMock<mtd::MockInputSendObserver> observer;

    MirEvent key_event;
    MirEvent motion_event;

    droidinput::MotionEvent client_motion_event;
    droidinput::KeyEvent client_key_event;

    testing::NiceMock<MockInputEventFactory> event_factory;

    droidinput::InputEvent * event= nullptr;
    uint32_t seq = 0;

    FakeScene fake_scene;
    mia::InputSender sender{mt::fake_shared(fake_scene), mt::fake_shared(loop), mt::fake_shared(observer), mr::null_input_report()};
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
    EXPECT_THROW(sender.send_event(key_event, channel), boost::exception);
}

TEST_F(AndroidInputSender,throws_on_deregistered_channels)
{
    register_surface();
    deregister_surface();

    EXPECT_THROW(sender.send_event(key_event, channel), boost::exception);
}

TEST_F(AndroidInputSender, first_send_on_surface_registers_server_fd)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, register_fd_handler(ElementsAre(channel->server_fd()),_,_));

    sender.send_event(key_event, channel);
}

TEST_F(AndroidInputSender, second_send_on_surface_does_not_register_server_fd)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, register_fd_handler(ElementsAre(channel->server_fd()),_,_)).Times(1);

    sender.send_event(key_event, channel);
    sender.send_event(key_event, channel);
}

TEST_F(AndroidInputSender, removal_of_surface_after_send_unregisters_server_fd)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, unregister_fd_handler(_)).Times(1);

    sender.send_event(key_event, channel);
    sender.send_event(key_event, channel);
    deregister_surface();

    Mock::VerifyAndClearExpectations(&loop);
}

TEST_F(AndroidInputSender, can_send_consumeable_mir_key_events)
{
    register_surface();

    sender.send_event(key_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, -1, &seq, &event));

    EXPECT_EQ(&client_key_event, event);
    EXPECT_EQ(key_event.key.scan_code, client_key_event.getScanCode());
}

TEST_F(AndroidInputSender, can_send_consumeable_mir_motion_events)
{
    using namespace ::testing;
    register_surface();

    sender.send_event(motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, -1, &seq, &event));

    EXPECT_EQ(&client_motion_event, event);
    EXPECT_EQ(motion_event.motion.pointer_count, client_motion_event.getPointerCount());
    EXPECT_EQ(motion_event.motion.device_id, client_motion_event.getDeviceId());
    EXPECT_EQ(0, client_motion_event.getXOffset());
    EXPECT_EQ(0, client_motion_event.getYOffset());

    auto const& expected_coords = motion_event.motion.pointer_coordinates;

    for (size_t i = 0; i != motion_event.motion.pointer_count; ++i)
    {
        EXPECT_EQ(expected_coords[i].x, client_motion_event.getRawX(i)) << "When i=" << i;
        EXPECT_EQ(expected_coords[i].x, client_motion_event.getX(i)) << "When i=" << i;
        EXPECT_EQ(expected_coords[i].y, client_motion_event.getRawY(i)) << "When i=" << i;
        EXPECT_EQ(expected_coords[i].y, client_motion_event.getY(i)) << "When i=" << i;
    }
}

TEST_F(AndroidInputSender, response_keeps_fd_registered)
{
    using namespace ::testing;
    register_surface();

    EXPECT_CALL(loop, unregister_fd_handler(_)).Times(0);

    sender.send_event(key_event, channel);
    consumer.consume(&event_factory, true, -1, &seq, &event);
    consumer.sendFinishedSignal(seq, true);
    loop.trigger_pending_fds();

    Mock::VerifyAndClearExpectations(&loop);
}

TEST_F(AndroidInputSender, finish_signal_triggers_success_callback_as_consumed)
{
    register_surface();

    sender.send_event(motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, -1, &seq, &event));
    EXPECT_CALL(observer,
                send_suceeded(mt::MirTouchEventMatches(motion_event),
                              &stub_surface,
                              mi::InputSendObserver::consumed));

    consumer.sendFinishedSignal(seq, true);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, finish_signal_triggers_success_callback_as_not_consumed)
{
    register_surface();

    sender.send_event(motion_event, channel);

    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, -1, &seq, &event));
    EXPECT_CALL(observer,
                send_suceeded(mt::MirTouchEventMatches(motion_event),
                              &stub_surface,
                              mi::InputSendObserver::not_consumed));

    consumer.sendFinishedSignal(seq, false);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, unordered_finish_signal_triggers_the_right_callback)
{
    register_surface();

    sender.send_event(motion_event, channel);
    sender.send_event(key_event, channel);

    uint32_t first_sequence, second_sequence;
    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, -1, &first_sequence, &event));
    EXPECT_EQ(droidinput::OK, consumer.consume(&event_factory, true, -1, &second_sequence, &event));

    EXPECT_CALL(observer,
                send_suceeded(mt::MirKeyEventMatches(key_event),
                              &stub_surface,
                              mi::InputSendObserver::consumed));
    EXPECT_CALL(observer,
                send_suceeded(mt::MirTouchEventMatches(motion_event),
                              &stub_surface,
                              mi::InputSendObserver::not_consumed));
    consumer.sendFinishedSignal(second_sequence, true);
    consumer.sendFinishedSignal(first_sequence, false);
    loop.trigger_pending_fds();
}

TEST_F(AndroidInputSender, observer_notified_on_disapeared_surface )
{
    register_surface();

    sender.send_event(key_event, channel);
    EXPECT_CALL(
        observer,
        send_failed(mt::MirKeyEventMatches(key_event), &stub_surface, mir::input::InputSendObserver::surface_disappeared));
    deregister_surface();
}

TEST_F(AndroidInputSender, alarm_created_for_input_send)
{
    using namespace ::testing;

    register_surface();

    EXPECT_CALL(loop, notify_in(_,_));
    sender.send_event(key_event, channel);
}

TEST_F(AndroidInputSender, observer_informed_on_response_timeout)
{
    register_surface();

    sender.send_event(key_event, channel);
    EXPECT_CALL(
        observer,
        send_failed(mt::MirKeyEventMatches(key_event), &stub_surface, mir::input::InputSendObserver::no_response_received));

    loop.fire_all_alarms();
}

TEST_F(AndroidInputSender, observer_informed_about_closed_socket_on_send_event)
{
    register_surface();

    EXPECT_CALL(
        observer,
        send_failed(mt::MirKeyEventMatches(key_event), &stub_surface, mir::input::InputSendObserver::socket_error));
    ::close(channel->client_fd());
    sender.send_event(key_event, channel);
}
