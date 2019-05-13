/*
 * Copyright © 2016-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */


#include "src/server/input/config_changer.h"
#include "src/server/scene/broadcasting_session_event_sink.h"
#include "mir/input/device.h"
#include "mir/scene/session_container.h"
#include "mir/scene/session_event_handler_register.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_touchscreen_config.h"

#include "mir/test/doubles/mock_input_device_hub.h"
#include "mir/test/doubles/mock_input_manager.h"
#include "mir/test/doubles/mock_device.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

namespace
{

struct FakeInputDeviceHub : mir::input::InputDeviceHub
{
    std::shared_ptr<mi::InputDeviceObserver> observer;
    std::string const first{"first"};
    std::string const second{"second"};
    MirInputDeviceId const first_id{13};
    MirInputDeviceId const second_id{14};
    mi::DeviceCapabilities const caps = mi::DeviceCapability::pointer | mi::DeviceCapability::touchpad |
                                        mi::DeviceCapability::keyboard | mi::DeviceCapability::alpha_numeric |
                                        mi::DeviceCapability::touchscreen;

    NiceMock<mtd::MockDevice> first_device{first_id, caps, first, first};
    NiceMock<mtd::MockDevice> second_device{second_id, caps, second, second};
    std::vector<std::shared_ptr<mir::input::Device>> active_devices;

    void add_observer(std::shared_ptr<mi::InputDeviceObserver> const& obs) override
    {
        observer = obs;
        observer->device_added(mt::fake_shared(first_device));
        observer->changes_complete();
    }
    void remove_observer(std::weak_ptr<mi::InputDeviceObserver> const&) override
    {
        observer.reset();
    }

    void for_each_input_device(std::function<void(mi::Device const& device)> const& callback) override
    {
        for (auto const & device : active_devices)
            callback(*device);
    }

    void for_each_mutable_input_device(std::function<void(mi::Device& device)> const& callback) override
    {
        for (auto & device : active_devices)
            callback(*device);
    }

    FakeInputDeviceHub()
    {
        active_devices.push_back(mt::fake_shared(first_device));
    }

    void add_second_device()
    {
        active_devices.push_back(mt::fake_shared(second_device));
        if (observer)
        {
            observer->device_added(mt::fake_shared(second_device));
            observer->changes_complete();
        }
    }
};

}

struct ConfigChanger : Test
{
    NiceMock<mtd::MockInputManager> mock_input_manager;
    FakeInputDeviceHub hub;
    ms::SessionContainer session_container;
    ms::BroadcastingSessionEventSink session_event_sink;

    mi::ConfigChanger changer{mt::fake_shared(mock_input_manager), mt::fake_shared(hub),
                              mt::fake_shared(session_container), mt::fake_shared(session_event_sink),
                              mt::fake_shared(hub)};

    auto get_full_device_conf(MirInputDeviceId id,
                              mi::DeviceCapabilities caps,
                              std::string const& name,
                              std::string const& unique_id)
    {
        MirInputDevice conf(id, caps, name, unique_id);
        conf.set_keyboard_config(MirKeyboardConfig{});
        conf.set_pointer_config(MirPointerConfig{});
        conf.set_touchpad_config(MirTouchpadConfig{});
        conf.set_touchscreen_config(MirTouchscreenConfig{});
        return conf;
    }
    auto get_populated_conf(MirInputDevice && conf)
    {
        MirInputConfig ret;
        ret.add_device_config(std::move(conf));
        return ret;
    }

    auto conf_for_first()
    {
        return get_full_device_conf(hub.first_id, hub.caps, hub.first, hub.first);
    }

    auto conf_for_second()
    {
        return get_full_device_conf(hub.second_id, hub.caps, hub.second, hub.second);
    }
};

TEST_F(ConfigChanger, doesnt_apply_config_for_unfocused_session)
{
    MirInputConfig conf{get_populated_conf(conf_for_first())};

    EXPECT_CALL(hub.first_device, apply_touchpad_configuration(_)).Times(0);
    EXPECT_CALL(hub.first_device, apply_keyboard_configuration(_)).Times(0);
    EXPECT_CALL(hub.first_device, apply_pointer_configuration(_)).Times(0);
    EXPECT_CALL(hub.first_device, apply_touchscreen_configuration(_)).Times(0);

    changer.configure(std::make_shared<mtd::StubSession>(), std::move(conf));
}

TEST_F(ConfigChanger, returns_updated_base_configuration_after_hardware_change)
{
    MirInputConfig conf{get_populated_conf(conf_for_first())};
    conf.add_device_config(conf_for_second());

    hub.observer->device_added(mt::fake_shared(hub.second_device));
    hub.observer->changes_complete();

    auto const base_conf = changer.base_configuration();
    EXPECT_THAT(base_conf, Eq(conf));
}

TEST_F(ConfigChanger, hardware_change_doesnt_apply_base_config_if_per_session_config_is_active)
{
    auto session1 = std::make_shared<mtd::StubSession>();

    changer.configure(session1, get_populated_conf(conf_for_first()));

    session_event_sink.handle_focus_change(session1);
    EXPECT_CALL(hub.first_device, apply_touchpad_configuration(_)).Times(0);
    EXPECT_CALL(hub.first_device, apply_keyboard_configuration(_)).Times(0);
    EXPECT_CALL(hub.first_device, apply_pointer_configuration(_)).Times(0);
    EXPECT_CALL(hub.second_device, apply_touchpad_configuration(_)).Times(0);
    EXPECT_CALL(hub.second_device, apply_keyboard_configuration(_)).Times(0);
    EXPECT_CALL(hub.second_device, apply_pointer_configuration(_)).Times(0);

    hub.observer->device_added(mt::fake_shared(hub.second_device));
    hub.observer->changes_complete();
}

TEST_F(ConfigChanger, notifies_all_sessions_on_hardware_config_change)
{
    mtd::MockSceneSession mock_session1;
    mtd::MockSceneSession mock_session2;

    session_container.insert_session(mt::fake_shared(mock_session1));
    session_container.insert_session(mt::fake_shared(mock_session2));

    EXPECT_CALL(mock_session1, send_input_config(_));
    EXPECT_CALL(mock_session2, send_input_config(_));

    hub.add_second_device();
}

TEST_F(ConfigChanger, focusing_a_session_with_attached_config_applies_config)
{
    auto changed_ptr_config = MirPointerConfig{mir_pointer_handedness_left, mir_pointer_acceleration_none, 0, 1, -1};
    auto changed_device_config = conf_for_first();
    changed_device_config.set_pointer_config(changed_ptr_config);

    auto changed_config = get_populated_conf(std::move(changed_device_config));
    auto session1 = std::make_shared<mtd::StubSession>();

    session_container.insert_session(session1);
    changer.configure(session1, std::move(changed_config));

    EXPECT_CALL(hub.first_device, apply_pointer_configuration(changed_ptr_config));

    session_event_sink.handle_focus_change(session1);
}

TEST_F(ConfigChanger, configuring_a_focused_session_sends_changed_config_to_client)
{
    auto session = std::make_shared<mtd::MockSceneSession>();
    session_container.insert_session(session);
    session_event_sink.handle_focus_change(session);

    auto changed_ptr_config = MirPointerConfig{mir_pointer_handedness_right, mir_pointer_acceleration_none, 0, -1, 1};
    auto changed_device_config = conf_for_first();
    changed_device_config.set_pointer_config(changed_ptr_config);

    auto changed_config = get_populated_conf(std::move(changed_device_config));

    EXPECT_CALL(*session, send_input_config(Eq(changed_config)));
    changer.configure(session, std::move(changed_config));
}

TEST_F(ConfigChanger, setting_the_base_configuration_changes_the_base_configuration)
{
    auto changed_ptr_config = MirPointerConfig{mir_pointer_handedness_left, mir_pointer_acceleration_none, 0, 1, -1};
    auto changed_device_config = conf_for_first();
    changed_device_config.set_pointer_config(changed_ptr_config);
    auto changed_config = get_populated_conf(std::move(changed_device_config));

    EXPECT_CALL(hub.first_device, apply_pointer_configuration(changed_ptr_config));

    changer.set_base_configuration(std::move(changed_config));
}

TEST_F(ConfigChanger, informs_input_manager_about_configuration_in_progess)
{
    EXPECT_CALL(mock_input_manager, pause_for_config());
    EXPECT_CALL(mock_input_manager, continue_after_config());

    changer.set_base_configuration(get_populated_conf(conf_for_first()));
}
