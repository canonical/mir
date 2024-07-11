/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/input/device_capability.h"
#include "src/platforms/evdev/libinput_ptr.h"
#include "src/platforms/evdev/libinput_device_ptr.h"
#include "src/platforms/evdev/libinput_device.h"
#include "src/server/report/null_report_factory.h"

#include "mir_test_framework/libinput_environment.h"
#include "mir/test/doubles/mock_led_observer_registrar.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>

namespace mtf = mir_test_framework;
namespace mi = mir::input;
namespace mie = mi::evdev;
namespace mt = mir::test;
namespace mtd = mt::doubles;

struct EvdevDeviceDetection : public ::testing::TestWithParam<std::tuple<std::string, mi::DeviceCapabilities>>
{
    mtf::LibInputEnvironment env;
};

TEST_P(EvdevDeviceDetection, evaluates_expected_input_class)
{
    using namespace testing;
    auto const& param = GetParam();
    auto dev = env.setup_device(std::get<0>(param));
    std::shared_ptr<libinput> lib = mie::make_libinput(nullptr);
    NiceMock<mtd::MockLedObserverRegistrar> led_observer_registrar;
    mie::LibInputDevice device(mir::report::null_input_report(), mie::make_libinput_device(lib, dev));
    auto info = device.get_device_info();
    EXPECT_THAT(info.capabilities, Eq(std::get<1>(param)));
}

INSTANTIATE_TEST_SUITE_P(InputDeviceCapabilityDetection,
                        EvdevDeviceDetection,
                        ::testing::Values(
                            std::make_tuple(
                                mtf::LibInputEnvironment::synaptics_touchpad,
                                mi::DeviceCapability::touchpad|mi::DeviceCapability::pointer
                                ),
                            std::make_tuple(
                                mtf::LibInputEnvironment::laptop_keyboard,
                                mi::DeviceCapability::keyboard|mi::DeviceCapability::alpha_numeric
                                ),
                            std::make_tuple(
                                mtf::LibInputEnvironment::usb_keyboard,
                                mi::DeviceCapability::keyboard|mi::DeviceCapability::alpha_numeric
                                ),
                            std::make_tuple(
                                mtf::LibInputEnvironment::usb_mouse,
                                mi::DeviceCapability::pointer
                                ),
                            std::make_tuple(
                                mtf::LibInputEnvironment::bluetooth_magic_trackpad,
                                mi::DeviceCapability::touchpad|mi::DeviceCapability::pointer
                                ),
                            std::make_tuple(
                                mtf::LibInputEnvironment::mtk_tpd, // device also reports available keys..
                                mi::DeviceCapabilities{mi::DeviceCapability::touchscreen}|
                                    mi::DeviceCapability::keyboard
                                ),
                            std::make_tuple(
                                mtf::LibInputEnvironment::usb_joystick,
                                mi::DeviceCapabilities{mi::DeviceCapability::joystick}|
                                    mi::DeviceCapability::keyboard
                                )
                            ));

