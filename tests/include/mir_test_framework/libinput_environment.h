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

#ifndef MIR_TEST_FRAMEWORK_LIBINPUT_ENVIRONMENT_H_
#define MIR_TEST_FRAMEWORK_LIBINPUT_ENVIRONMENT_H_

#include "mir/test/doubles/mock_udev.h"
#include "mir/test/doubles/mock_libinput.h"

#include <unordered_map>
#include <string>


namespace mir_test_framework
{

struct LibInputEnvironment
{
    libinput *li_context{reinterpret_cast<libinput*>(0xFEBA)};

    LibInputEnvironment();

    /**
     * Creates a predefined libinput device selected via the given device_name. It will then
     * discoverable through udev and libinput.
     * \note This method will not queue a LIBINPUT_EVENT_DEVICE_ADDED event.
     */
    libinput_device* setup_device(std::string const& device_name);
    /**
     * Creates a predefined libinput device selected via the given device_name. It will then
     * discoverable through udev and libinput. The device can be reached by processing the
     * queued LIBINPUT_EVENT_DEVICE_ADDED event.
     */
    void add_standard_device(std::string const& device_name);
    /**
     * Removes the predefined device selected via the given device_name.
     */
    void remove_standard_device(std::string const& device_name);

    testing::NiceMock<mir::test::doubles::MockLibInput> mock_libinput;
    testing::NiceMock<mir::test::doubles::MockUdev> mock_udev;

    template<typename PtrT>
    PtrT to_fake_ptr(unsigned int number)
    {
        return reinterpret_cast<PtrT>(number);
    }

    template<typename PtrT, typename Container>
    PtrT get_next_fake_ptr(Container const& container)
    {
        return to_fake_ptr<PtrT>(container.size()+1);
    }

    struct DeviceSetup
    {
        DeviceSetup() = default;
        explicit DeviceSetup(std::string const& path) : path(path) {}
        std::string path;
        std::unordered_map<std::string, std::string> properties;
    };

    std::unordered_map<std::string, DeviceSetup> standard_devices;
    static std::string const synaptics_touchpad;
    static std::string const bluetooth_magic_trackpad;
    static std::string const usb_keyboard;
    static std::string const usb_mouse;
    static std::string const laptop_keyboard;
    static std::string const mtk_tpd;
    static std::string const usb_joystick;
    std::unordered_map<std::string, libinput_device*> available_devs;
};

}

#endif
