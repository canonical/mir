/*
 * Copyright © 2015 Canonical Ltd.
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

#include "src/platforms/evdev/platform.h"
#include "src/server/report/null_report_factory.h"

#include "mir/input/input_device_registry.h"
#include "mir/dispatch/dispatchable.h"

#include "mir/udev/wrapper.h"
#include "mir_test_framework/libinput_environment.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_libinput.h"
#include "mir/test/doubles/mock_udev.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <umockdev.h>
#include <memory>
#include <vector>
#include <initializer_list>

namespace mi = mir::input;
namespace mie = mi::evdev;
namespace mr = mir::report;
namespace mu = mir::udev;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

using namespace ::testing;
namespace
{

struct MockInputDeviceRegistry : public mi::InputDeviceRegistry
{
    MOCK_METHOD1(add_device, void(std::shared_ptr<mi::InputDevice> const&));
    MOCK_METHOD1(remove_device, void(std::shared_ptr<mi::InputDevice> const&));
};

struct EvdevInputPlatform : public ::testing::TestWithParam<std::string>
{
    testing::NiceMock<MockInputDeviceRegistry> mock_registry;
    mtf::LibInputEnvironment env;

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

    void setup_groupped_devices(std::initializer_list<libinput_device*> devices)
    {
        auto next_group = env.mock_libinput.get_next_fake_ptr<libinput_device_group*>();

        for (auto dev : devices)
            ON_CALL(env.mock_libinput, libinput_device_get_device_group(dev)).WillByDefault(Return(next_group));
    }

    auto create_input_platform()
    {
        auto ctx = std::make_unique<mu::Context>();
        return std::make_unique<mie::Platform>(mt::fake_shared(mock_registry), mr::null_input_report(), std::move(ctx));
    }
};

void run_dispatchable(mir::input::Platform& platform)
{
    platform.dispatchable()->dispatch(mir::dispatch::FdEvent::readable);
}

}

TEST_P(EvdevInputPlatform, scans_on_start)
{
    env.add_standard_device(GetParam());

    using namespace ::testing;
    auto platform = create_input_platform();

    EXPECT_CALL(mock_registry, add_device(_));

    platform->start();
    run_dispatchable(*platform);
}

TEST_P(EvdevInputPlatform, detects_on_hotplug)
{
    using namespace ::testing;
    auto platform = create_input_platform();

    platform->start();

    {
        EXPECT_CALL(mock_registry, add_device(_));
        env.add_standard_device(GetParam());

        run_dispatchable(*platform);
    }
}

TEST_P(EvdevInputPlatform, detects_hot_removal)
{
    using namespace ::testing;
    auto platform = create_input_platform();

    platform->start();

    {
        EXPECT_CALL(mock_registry, add_device(_));
        EXPECT_CALL(mock_registry, remove_device(_));

        env.add_standard_device(GetParam());
        run_dispatchable(*platform);
        env.remove_standard_device(GetParam());
        run_dispatchable(*platform);
    }
}

TEST_P(EvdevInputPlatform, removes_devices_on_stop)
{
    using namespace ::testing;
    auto platform = create_input_platform();

    env.add_standard_device(GetParam());
    platform->start();

    run_dispatchable(*platform);
    EXPECT_CALL(mock_registry, remove_device(_));
    platform->stop();
}

INSTANTIATE_TEST_CASE_P(DeviceHandling,
                        EvdevInputPlatform,
                        ::testing::Values(mtf::LibInputEnvironment::synaptics_touchpad,
                                          mtf::LibInputEnvironment::usb_keyboard,
                                          mtf::LibInputEnvironment::usb_mouse,
                                          mtf::LibInputEnvironment::mtk_tpd,
                                          mtf::LibInputEnvironment::bluetooth_magic_trackpad));

TEST_F(EvdevInputPlatform, register_ungrouped_devices)
{
    auto platform = create_input_platform();
    platform->start();

    EXPECT_CALL(mock_registry, add_device(_)).Times(2);

    env.add_standard_device(mtf::LibInputEnvironment::synaptics_touchpad);
    env.add_standard_device(mtf::LibInputEnvironment::usb_keyboard);
    run_dispatchable(*platform);
}

TEST_F(EvdevInputPlatform, ignore_devices_from_same_group)
{
    auto platform = create_input_platform();
    platform->start();

    EXPECT_CALL(mock_registry, add_device(_)).Times(1);

    auto touch_pad = env.setup_device(mtf::LibInputEnvironment::synaptics_touchpad);
    auto keyboard = env.setup_device(mtf::LibInputEnvironment::usb_keyboard);
    setup_groupped_devices({touch_pad, keyboard});
    env.mock_libinput.setup_device_add_event(touch_pad);
    env.mock_libinput.setup_device_add_event(keyboard);
    run_dispatchable(*platform);
}

TEST_F(EvdevInputPlatform, creates_new_context_on_resume)
{
    using namespace ::testing;
    auto platform = create_input_platform();

    platform->start();
    platform->stop();

    EXPECT_CALL(env.mock_libinput, libinput_path_create_context(_,_));
    platform->start();
}
