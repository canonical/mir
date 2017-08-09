/*
 * Copyright © 2017 Canonical Ltd.
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

#include "src/server/input/default_input_device_hub.h"
#include "mir/test/doubles/mock_input_device_hub.h"
#include "mir/test/doubles/mock_device.h"
#include "mir/test/doubles/mock_input_device_observer.h"
#include "mir/test/doubles/triggered_main_loop.h"
#include "mir/test/fake_shared.h"

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mi = mir::input;
using namespace testing;

namespace
{

struct WrappedHub : mtd::MockInputDeviceHub
{
    std::shared_ptr<mi::InputDeviceObserver> external_hub;
    void add_observer(std::shared_ptr<mi::InputDeviceObserver> const& observer)
    {
        external_hub = observer;
    }
};

struct ExternalInputDeviceHub : ::testing::Test
{
    mtd::TriggeredMainLoop server_actions;
    std::shared_ptr<WrappedHub> wrapped_hub{std::make_shared<WrappedHub>()};
    mi::ExternalInputDeviceHub hub{wrapped_hub, mt::fake_shared(server_actions)};
    NiceMock<mtd::MockInputDeviceObserver> mock_observer;

    NiceMock<mtd::MockDevice> device{1, mi::DeviceCapability::unknown, "name", "name-id-1"};
    NiceMock<mtd::MockDevice> another_device{2, mi::DeviceCapability::keyboard, "keyboard", "keyboard-id-2"};

    void submit_device_to_hub(std::shared_ptr<mi::Device> const& dev)
    {
        wrapped_hub->external_hub->device_added(dev);
        wrapped_hub->external_hub->changes_complete();
    }

    void remove_device_to_hub(std::shared_ptr<mi::Device> const& dev)
    {
        wrapped_hub->external_hub->device_removed(dev);
        wrapped_hub->external_hub->changes_complete();
    }
};
}

TEST_F(ExternalInputDeviceHub, is_observer_to_wrapped_hub)
{
    EXPECT_THAT(wrapped_hub->external_hub, Ne(nullptr));
}

TEST_F(ExternalInputDeviceHub, new_observer_is_called_through_server_action)
{
    EXPECT_CALL(mock_observer, changes_complete()).Times(1);
    hub.add_observer(mt::fake_shared(mock_observer));
    server_actions.trigger_server_actions();
}

TEST_F(ExternalInputDeviceHub, informs_observer_about_existing_devices)
{
    submit_device_to_hub(mt::fake_shared(device));
    submit_device_to_hub(mt::fake_shared(another_device));

    EXPECT_CALL(mock_observer, device_added(_)).Times(2);
    EXPECT_CALL(mock_observer, changes_complete()).Times(1);
    hub.add_observer(mt::fake_shared(mock_observer));
    server_actions.trigger_server_actions();
}

TEST_F(ExternalInputDeviceHub, informs_observer_about_added_devices)
{
    InSequence seq;

    EXPECT_CALL(mock_observer, changes_complete());
    EXPECT_CALL(mock_observer, device_added(_));
    EXPECT_CALL(mock_observer, changes_complete());
    hub.add_observer(mt::fake_shared(mock_observer));
    server_actions.trigger_server_actions();

    submit_device_to_hub(mt::fake_shared(device));

    server_actions.trigger_server_actions();
}

TEST_F(ExternalInputDeviceHub, triggers_observer_on_removed_devices)
{
    submit_device_to_hub(mt::fake_shared(device));

    EXPECT_CALL(mock_observer, device_added(_)).Times(1);
    EXPECT_CALL(mock_observer, changes_complete()).Times(1);
    hub.add_observer(mt::fake_shared(mock_observer));
    server_actions.trigger_server_actions();

    EXPECT_CALL(mock_observer, device_removed(_)).Times(1);
    EXPECT_CALL(mock_observer, changes_complete()).Times(1);

    remove_device_to_hub(mt::fake_shared(device));
    server_actions.trigger_server_actions();
}
