/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir_test_framework/libinput_environment.h"
#include "mir/test/fake_shared.h"

#include <boost/throw_exception.hpp>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
using namespace testing;

using DeviceEntry = std::pair<std::string, mtf::LibInputEnvironment::DeviceSetup>;

std::string const mtf::LibInputEnvironment::synaptics_touchpad("synaptics-touchpad");
std::string const mtf::LibInputEnvironment::bluetooth_magic_trackpad("bluetooth-magic-trackpad");
std::string const mtf::LibInputEnvironment::usb_keyboard("usb-keyboard");
std::string const mtf::LibInputEnvironment::usb_mouse("usb-mouse");
std::string const mtf::LibInputEnvironment::laptop_keyboard("laptop-keyboard");
std::string const mtf::LibInputEnvironment::mtk_tpd("mtk-tpd");
std::string const mtf::LibInputEnvironment::usb_joystick("usb_joystick");

namespace
{
std::string const model("ID_MODEL");
std::string const input("ID_INPUT");
std::string const touchpad("ID_INPUT_TOUCHPAD");
std::string const touchscreen("ID_INPUT_TOUCHSCREEN");
std::string const mouse("ID_INPUT_MOUSE");
std::string const key("ID_INPUT_KEY");
std::string const keyboard("ID_INPUT_KEYBOARD");
std::string const joystick("ID_INPUT_JOYSTICK");
}

mtf::LibInputEnvironment::LibInputEnvironment()
    : standard_devices{DeviceEntry{synaptics_touchpad, DeviceSetup{"/dev/input/event2", }},
                       DeviceEntry{usb_keyboard, DeviceSetup{"/dev/input/event11"}},
                       DeviceEntry{usb_mouse, DeviceSetup{"/dev/input/event12"}},
                       DeviceEntry{laptop_keyboard, DeviceSetup{"/dev/input/event4"}},
                       DeviceEntry{bluetooth_magic_trackpad, DeviceSetup{"/dev/input/event13"}},
                       DeviceEntry{usb_joystick, DeviceSetup{"/dev/input/event15"}},
                       DeviceEntry{mtk_tpd, DeviceSetup{"/dev/input/event1"}}}
{
    standard_devices[synaptics_touchpad].properties[touchpad] = "1";
    standard_devices[bluetooth_magic_trackpad].properties[touchpad]="1";

    standard_devices[usb_keyboard].properties[key]="1";
    standard_devices[usb_keyboard].properties[keyboard]="1";

    standard_devices[usb_mouse].properties[mouse]="1";

    standard_devices[laptop_keyboard].properties[key]="1";
    standard_devices[laptop_keyboard].properties[keyboard]="1";

    standard_devices[mtk_tpd].properties[touchscreen]="1";
    standard_devices[mtk_tpd].properties[key]="1";

    standard_devices[usb_joystick].properties[joystick]="1";
    standard_devices[usb_joystick].properties[key]="1";

    for (auto device : standard_devices)
    {
        device.second.properties[input] = "1";
        device.second.properties[model] = device.first;
    }

    ON_CALL(mock_libinput, libinput_path_create_context(_,_))
        .WillByDefault(Return(li_context));
    ON_CALL(mock_libinput, libinput_device_get_device_group(_))
            .WillByDefault(Return(nullptr));

}

void mtf::LibInputEnvironment::add_standard_device(std::string const& device_name)
{
    auto dev = setup_device(device_name);

    mock_libinput.setup_device_add_event(dev);
}

void mtf::LibInputEnvironment::remove_standard_device(std::string const& device_name)
{
    mock_libinput.setup_device_remove_event(available_devs[device_name]);
    available_devs.erase(device_name);
}

libinput_device* mtf::LibInputEnvironment::setup_device(std::string const& device_name)
{
    auto entry = standard_devices.find(device_name);
    if (entry == standard_devices.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("device not known"));

    auto dev = mock_libinput.get_next_fake_ptr<libinput_device*>();
    auto group = mock_libinput.get_next_fake_ptr<libinput_device_group*>();
    auto u_dev = std::shared_ptr<udev_device>{
        mock_libinput.get_next_fake_ptr<udev_device*>(),
        [](auto) {}};
    auto const devnum = mock_libinput.get_next_devnum();

    mock_libinput.setup_device(dev, group, u_dev, device_name.c_str(), entry->second.path.c_str(), 123, 456);
    ON_CALL(mock_udev, udev_device_get_devnode(u_dev.get()))
        .WillByDefault(Return(entry->second.path.c_str()));
    ON_CALL(mock_udev, udev_device_get_property_value(u_dev.get(), _))
        .WillByDefault(Invoke(
                [this, device_name](udev_device*, char const* property)
                {
                    return standard_devices[device_name].properties[property].c_str();
                }
                ));
    ON_CALL(mock_udev, udev_device_get_devnum(u_dev.get()))
        .WillByDefault(Return(devnum));

    available_devs[device_name] = dev;

    return dev;
}
