/*
 * Copyright © 2015 Canonical Ltd.
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

#include "src/platforms/evdev/platform.h"
#include "src/server/report/null_report_factory.h"

#include "mir/input/input_device_registry.h"
#include "mir/dispatch/dispatchable.h"

#include "mir/udev/wrapper.h"
#include "mir_test_framework/udev_environment.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_libinput.h"

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

using ::testing::_;
namespace
{

struct MockInputDeviceRegistry : public mi::InputDeviceRegistry
{
    MOCK_METHOD1(add_device, void(std::shared_ptr<mi::InputDevice> const&));
    MOCK_METHOD1(remove_device, void(std::shared_ptr<mi::InputDevice> const&));
};

struct EvdevInputPlatform : public ::testing::TestWithParam<std::string>
{
    mir_test_framework::UdevEnvironment env;
    testing::NiceMock<mtd::MockLibInput> mock_libinput;
    testing::NiceMock<MockInputDeviceRegistry> mock_registry;
    libinput *li_context{reinterpret_cast<libinput*>(0xFEBA)};

    std::vector<std::pair<std::string, libinput_device*>> devices;
    std::vector<std::pair<libinput_device_group*, std::vector<libinput_device*>>> groups;

    EvdevInputPlatform()
    {
        using namespace testing;
        ON_CALL(mock_libinput, libinput_path_create_context(_,_))
            .WillByDefault(Return(li_context));
        ON_CALL(mock_libinput, libinput_path_add_device(li_context,_))
            .WillByDefault(Invoke(
                    [this](libinput const*, char const* path) -> libinput_device*
                    {
                        return open_device_from_path(path);
                    }
                    ));
        ON_CALL(mock_libinput, libinput_device_get_device_group(_))
            .WillByDefault(Invoke(
                    [this](libinput_device *dev)
                    {
                        return get_device_group(dev);
                    }));
    }

    auto get_device_group(libinput_device *dev) -> libinput_device_group*
    {
        auto it = find_if(begin(groups),
                          end(groups),
                          [dev] (auto const& item)
                          {
                              return end(item.second) != find(begin(item.second), end(item.second), dev);
                          });
        if (end(groups) == it)
        {
            groups.emplace_back(get_next_fake_ptr<libinput_device_group*>(groups), std::vector<libinput_device*>{dev});
            return groups.back().first;
        }

        return it->first;
    }


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
        groups.emplace_back(get_next_fake_ptr<libinput_device_group*>(groups), devices);
    }

    auto open_device_from_path(char const* path) -> libinput_device*
    {
        auto it = find_if(begin(devices),
                          end(devices),
                          [path] (auto const& item)
                          {
                              return item.first == path;
                          });
        if (end(devices) == it)
        {
            auto dev_ptr = get_next_fake_ptr<libinput_device*>(devices);
            devices.emplace_back(path, dev_ptr);
            auto const& dev_entry = devices.back();
            mock_libinput.setup_device(li_context, dev_ptr, dev_entry.first.c_str(), dev_entry.first.c_str(), 123, 456);
            return devices.back().second;
        }

        return it->second;
    }

    auto create_input_platform()
    {
        auto ctx = std::make_unique<mu::Context>();
        auto monitor = std::make_unique<mu::Monitor>(*ctx.get());
        return std::make_unique<mie::Platform>(mt::fake_shared(mock_registry), mr::null_input_report(), std::move(ctx),
                                               std::move(monitor));
    }

    void remove_devices()
    {
        mir::udev::Enumerator devices{std::make_shared<mir::udev::Context>()};
        devices.scan_devices();

        for (auto& device : devices)
        {
            /*
             * Remove just the device providing dev/input/event*
             * If we remove more, it's possible that we'll remove the parent of the
             * /dev/input device, and umockdev will not generate a remove event
             * in that case.
             */
            if (device.devnode() && (std::string(device.devnode()).find("input/event") != std::string::npos))
            {
                env.remove_device((std::string("/sys") + device.devpath()).c_str());
            }
        }
    }
};

}

inline void run_dispatchable( mir::input::Platform& platform)
{
    platform.dispatchable()->dispatch(mir::dispatch::FdEvent::readable);
}

TEST_P(EvdevInputPlatform, scans_on_start)
{
    env.add_standard_device(GetParam());

    using namespace ::testing;
    auto platform = create_input_platform();

    EXPECT_CALL(mock_registry, add_device(_));

    platform->start();
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
        remove_devices();
        run_dispatchable(*platform);
    }
}

TEST_P(EvdevInputPlatform, removes_devices_on_stop)
{
    using namespace ::testing;
    auto platform = create_input_platform();

    env.add_standard_device(GetParam());
    platform->start();

    EXPECT_CALL(mock_registry, remove_device(_));
    platform->stop();
}

INSTANTIATE_TEST_CASE_P(DeviceHandling,
                        EvdevInputPlatform,
                        ::testing::Values(std::string("synaptics-touchpad"),
                                          std::string("usb-keyboard"),
                                          std::string("usb-mouse"),
                                          std::string("laptop-keyboard"),
                                          std::string("bluetooth-magic-trackpad")));



TEST_F(EvdevInputPlatform, register_ungrouped_devices)
{
    auto platform = create_input_platform();
    platform->start();

    EXPECT_CALL(mock_registry, add_device(_)).Times(2);

    env.add_standard_device("synaptics-touchpad");
    env.add_standard_device("usb-keyboard");
    run_dispatchable(*platform);
}

TEST_F(EvdevInputPlatform, ignore_devices_from_same_group)
{
    auto platform = create_input_platform();
    platform->start();

    EXPECT_CALL(mock_registry, add_device(_)).Times(1);

    setup_groupped_devices({to_fake_ptr<libinput_device*>(1), to_fake_ptr<libinput_device*>(2)});

    env.add_standard_device("synaptics-touchpad");
    env.add_standard_device("usb-keyboard");
    run_dispatchable(*platform);
}

TEST_F(EvdevInputPlatform, ignore_non_input_devices)
{
    auto platform = create_input_platform();
    platform->start();

    EXPECT_CALL(mock_registry, add_device(_)).Times(0);
    EXPECT_CALL(mock_libinput, libinput_path_add_device(_,_)).Times(0);

    env.add_standard_device("standard-drm-devices");
    run_dispatchable(*platform);
}
