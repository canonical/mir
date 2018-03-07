/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_test_framework/udev_environment.h"
#include "mir/udev/wrapper.h"
#include "mir/test/death.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <stdexcept>
#include <umockdev.h>
#include <libudev.h>
#include <poll.h>
#include <sys/sysmacros.h>

namespace mtf=mir_test_framework;

namespace
{

bool KilledByInvalidMemoryAccess(int exit_status)
{
    return testing::KilledBySignal(SIGSEGV)(exit_status) ||
           testing::KilledBySignal(SIGBUS)(exit_status) ||
           testing::KilledBySignal(SIGABRT)(exit_status) ||
           // It seems that valgrind kills us with SIGKILL
           testing::KilledBySignal(SIGKILL)(exit_status);
}

class UdevWrapperTest : public ::testing::Test
{
public:
    mtf::UdevEnvironment udev_environment;
};

}

TEST_F(UdevWrapperTest, IteratesOverCorrectNumberOfDevices)
{
    udev_environment.add_device("drm", "fakedev1", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev2", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev3", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev4", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev5", NULL, {}, {});

    auto ctx = std::make_shared<mir::udev::Context>();
    mir::udev::Enumerator enumerator(ctx);

    enumerator.scan_devices();

    int device_count = 0;
    for (auto& device : enumerator)
    {
        // Silence unused variable warning
        static_cast<void>(device);
        ++device_count;
    }

    ASSERT_EQ(device_count, 5);
}

TEST_F(UdevWrapperTest, EnumeratorMatchSubsystemIncludesCorrectDevices)
{
    udev_environment.add_device("drm", "fakedrm1", NULL, {}, {});
    udev_environment.add_device("scsi", "fakescsi1", NULL, {}, {});
    udev_environment.add_device("drm", "fakedrm2", NULL, {}, {});
    udev_environment.add_device("usb", "fakeusb1", NULL, {}, {});
    udev_environment.add_device("usb", "fakeusb2", NULL, {}, {});

    auto ctx = std::make_shared<mir::udev::Context>();
    mir::udev::Enumerator devices(ctx);

    devices.match_subsystem("drm");
    devices.scan_devices();
    for (auto& device : devices)
    {
        ASSERT_STREQ("drm", device.subsystem());
    }
}

TEST_F(UdevWrapperTest, UdevDeviceHasCorrectDevType)
{
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {}, {"DEVTYPE", "drm_minor"});

    mir::udev::Context ctx;
    auto dev = ctx.device_from_syspath(sysfs_path);
    ASSERT_STREQ("drm_minor", dev->devtype());
}

TEST_F(UdevWrapperTest, UdevDeviceHasCorrectDevPath)
{
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {}, {});

    mir::udev::Context ctx;
    auto dev = ctx.device_from_syspath(sysfs_path);
    ASSERT_STREQ("/devices/card0", dev->devpath());
}

TEST_F(UdevWrapperTest, UdevDeviceHasCorrectDevNode)
{
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {}, {"DEVNAME", "/dev/dri/card0"});

    mir::udev::Context ctx;
    auto dev = ctx.device_from_syspath(sysfs_path);

    ASSERT_STREQ("/dev/dri/card0", dev->devnode());
}

TEST_F(UdevWrapperTest, UdevDeviceHasCorrectProperties)
{
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {},
        {"AWESOME", "yes", "REALLY", "absolutely"});

    mir::udev::Context ctx;
    auto dev = ctx.device_from_syspath(sysfs_path);

    ASSERT_STREQ("yes", dev->property("AWESOME"));
    ASSERT_STREQ("absolutely", dev->property("REALLY"));
    ASSERT_EQ(NULL, dev->property("SOMETHING_ELSE"));
}

TEST_F(UdevWrapperTest, UdevDeviceHasCorrectMajorMinorNumbers)
{
    using namespace testing;
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {},
                                                  {"MAJOR", "22", "MINOR", "42"});

    mir::udev::Context ctx;
    auto dev = ctx.device_from_syspath(sysfs_path);

    dev_t devnum = dev->devnum();
    EXPECT_THAT(major(devnum), Eq(22));
    EXPECT_THAT(minor(devnum), Eq(42));
}


TEST_F(UdevWrapperTest, UdevDeviceComparisonIsReflexive)
{
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {}, {});

    mir::udev::Context ctx;
    auto dev = ctx.device_from_syspath(sysfs_path);

    EXPECT_TRUE(*dev == *dev);
}

TEST_F(UdevWrapperTest, UdevDeviceComparisonIsSymmetric)
{
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {}, {});

    mir::udev::Context ctx;
    std::shared_ptr<mir::udev::Device> same_one = ctx.device_from_syspath(sysfs_path);
    std::shared_ptr<mir::udev::Device> same_two = ctx.device_from_syspath(sysfs_path);

    EXPECT_TRUE(*same_one == *same_two);
    EXPECT_TRUE(*same_two == *same_one);
}

TEST_F(UdevWrapperTest, UdevDeviceDifferentDevicesCompareFalse)
{
    auto path_one = udev_environment.add_device("drm", "card0", NULL, {}, {});
    auto path_two = udev_environment.add_device("drm", "card1", NULL, {}, {});

    mir::udev::Context ctx;
    auto dev_one = ctx.device_from_syspath(path_one);
    auto dev_two = ctx.device_from_syspath(path_two);

    EXPECT_FALSE(*dev_one == *dev_two);
    EXPECT_FALSE(*dev_two == *dev_one);
}

TEST_F(UdevWrapperTest, UdevDeviceDifferentDevicesAreNotEqual)
{
    auto path_one = udev_environment.add_device("drm", "card0", NULL, {}, {});
    auto path_two = udev_environment.add_device("drm", "card1", NULL, {}, {});

    mir::udev::Context ctx;
    auto dev_one = ctx.device_from_syspath(path_one);
    auto dev_two = ctx.device_from_syspath(path_two);

    EXPECT_TRUE(*dev_one != *dev_two);
    EXPECT_TRUE(*dev_two != *dev_one);
}

TEST_F(UdevWrapperTest, UdevDeviceSameDeviceIsNotNotEqual)
{
    auto sysfs_path = udev_environment.add_device("drm", "card0", NULL, {}, {});

    mir::udev::Context ctx;
    auto same_one = ctx.device_from_syspath(sysfs_path);
    auto same_two = ctx.device_from_syspath(sysfs_path);

    EXPECT_FALSE(*same_one != *same_two);
    EXPECT_FALSE(*same_two != *same_one);
}

TEST_F(UdevWrapperTest, EnumeratorMatchParentMatchesOnlyChildren)
{
    auto card0_syspath = udev_environment.add_device("drm", "card0", NULL, {}, {});
    udev_environment.add_device("usb", "fakeusb", NULL, {}, {});

    udev_environment.add_device("drm", "card0-HDMI1", "/sys/devices/card0", {}, {});
    udev_environment.add_device("drm", "card0-VGA1", "/sys/devices/card0", {}, {});
    udev_environment.add_device("drm", "card0-LVDS1", "/sys/devices/card0", {}, {});

    auto ctx = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator devices(ctx);
    auto drm_device = ctx->device_from_syspath(card0_syspath);

    devices.match_parent(*drm_device);
    devices.scan_devices();

    int child_count = 0;
    for (auto& device : devices)
    {
        EXPECT_STREQ("drm", device.subsystem());
        ++child_count;
    }
    EXPECT_EQ(4, child_count);
}

TEST_F(UdevWrapperTest, EnumeratorThrowsLogicErrorIfIteratedBeforeScanned)
{
    auto ctx = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator devices(ctx);

    EXPECT_THROW({ devices.begin(); },
                 std::logic_error);
}

TEST_F(UdevWrapperTest, EnumeratorLogicErrorHasSensibleMessage)
{
    auto ctx = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator devices(ctx);
    std::string error_msg;

    try
    {
        devices.begin();
    }
    catch (std::logic_error& e)
    {
        error_msg = e.what();
    }
    EXPECT_STREQ("Attempted to iterate over udev devices without first scanning", error_msg.c_str());
}

TEST_F(UdevWrapperTest, EnumeratorEnumeratesEmptyList)
{
    auto ctx = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator devices(ctx);

    devices.scan_devices();

    for (auto& device : devices)
        ADD_FAILURE() << "Unexpected udev device: " << device.devpath();
}

TEST_F(UdevWrapperTest, EnumeratorAddMatchSysnameIncludesCorrectDevices)
{
    auto drm_sysfspath = udev_environment.add_device("drm", "card0", NULL, {}, {});
    udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.add_device("drm", "card0-LVDS1", drm_sysfspath.c_str(), {}, {});
    udev_environment.add_device("drm", "card1", NULL, {}, {});

    auto ctx = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator devices(ctx);

    devices.match_sysname("card[0-9]");
    devices.scan_devices();
    for (auto& device : devices)
    {
        const char *start = strstr(device.devpath(), "card");
        ASSERT_TRUE(start);
        int num;
        EXPECT_EQ(1, sscanf(start, "card%d", &num))
            << "Unexpected device with devpath:" << device.devpath();
    }
}

typedef UdevWrapperTest UdevWrapperDeathTest;

TEST_F(UdevWrapperDeathTest, DereferencingEndReturnsInvalidObject)
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.add_device("drm", "card1", NULL, {}, {});

    mir::udev::Enumerator devices(std::make_shared<mir::udev::Context>());

    devices.scan_devices();

    MIR_EXPECT_EXIT((*devices.end()).subsystem(), KilledByInvalidMemoryAccess, "");

    auto iter = devices.begin();

    while(iter != devices.end())
    {
        iter++;
    }
    MIR_EXPECT_EXIT((*iter).subsystem(), KilledByInvalidMemoryAccess, "");
}

TEST_F(UdevWrapperTest, MemberDereferenceWorks)
{
    udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.add_device("drm", "card1", NULL, {}, {});

    mir::udev::Enumerator devices(std::make_shared<mir::udev::Context>());

    devices.scan_devices();
    auto iter = devices.begin();

    EXPECT_STREQ("drm", iter->subsystem());
    EXPECT_STREQ("drm", iter->subsystem());
}

TEST_F(UdevWrapperDeathTest, MemberDereferenceOfEndDies)
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.add_device("drm", "card1", NULL, {}, {});

    mir::udev::Enumerator devices(std::make_shared<mir::udev::Context>());

    devices.scan_devices();

    MIR_EXPECT_EXIT(devices.end()->subsystem(), KilledByInvalidMemoryAccess, "");

    auto iter = devices.begin();

    while(iter != devices.end())
    {
        iter++;
    }
    MIR_EXPECT_EXIT(iter->subsystem(), KilledByInvalidMemoryAccess, "");
}

TEST_F(UdevWrapperTest, UdevMonitorDoesNotTriggerBeforeEnabling)
{
    mir::udev::Monitor monitor{mir::udev::Context()};
    bool event_handler_called = false;

    udev_environment.add_device("drm", "control64D", NULL, {}, {});

    monitor.process_events([&event_handler_called](mir::udev::Monitor::EventType,
                                                   mir::udev::Device const&) {event_handler_called = true;});

    EXPECT_FALSE(event_handler_called);
}

TEST_F(UdevWrapperTest, UdevMonitorTriggersAfterEnabling)
{
    mir::udev::Monitor monitor{mir::udev::Context()};
    bool event_handler_called = false;

    monitor.enable();

    udev_environment.add_device("drm", "control64D", NULL, {}, {});

    monitor.process_events([&event_handler_called](mir::udev::Monitor::EventType,
                                                   mir::udev::Device const&) {event_handler_called = true;});

    EXPECT_TRUE(event_handler_called);
}

TEST_F(UdevWrapperTest, UdevMonitorSendsRemoveEvent)
{
    mir::udev::Monitor monitor{mir::udev::Context()};
    bool remove_event_received = false;

    monitor.enable();

    auto test_sysfspath = udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.remove_device(test_sysfspath);

    monitor.process_events([&remove_event_received]
        (mir::udev::Monitor::EventType action, mir::udev::Device const&)
            {
                if (action == mir::udev::Monitor::EventType::REMOVED)
                    remove_event_received = true;
            });

    EXPECT_TRUE(remove_event_received);
}

TEST_F(UdevWrapperTest, UdevMonitorSendsChangedEvent)
{
    mir::udev::Monitor monitor{mir::udev::Context()};
    bool changed_event_received = false;

    monitor.enable();

    auto test_sysfspath = udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.emit_device_changed(test_sysfspath);

    monitor.process_events([&changed_event_received]
        (mir::udev::Monitor::EventType action, mir::udev::Device const&)
            {
                if (action == mir::udev::Monitor::EventType::CHANGED)
                    changed_event_received = true;
            });

    EXPECT_TRUE(changed_event_received);
}

TEST_F(UdevWrapperTest, UdevMonitorEventHasCorrectDeviceDetails)
{
    mir::udev::Context ctx;

    mir::udev::Monitor monitor{mir::udev::Context()};
    bool event_handler_called = false;

    monitor.enable();

    auto sysfs_path = udev_environment.add_device("drm", "control64D", NULL, {}, {});
    auto device = ctx.device_from_syspath(sysfs_path);

    monitor.process_events(
        [&event_handler_called, device](mir::udev::Monitor::EventType, mir::udev::Device const& dev)
            {
                event_handler_called = true;
                EXPECT_EQ(*device, dev);
            });

    ASSERT_TRUE(event_handler_called);
}

TEST_F(UdevWrapperTest, UdevMonitorFdIsReadableWhenEventsAvailable)
{
    mir::udev::Monitor monitor{mir::udev::Context()};

    monitor.enable();

    udev_environment.add_device("drm", "card0", NULL, {}, {});

    struct pollfd fds;
    fds.fd = monitor.fd();
    fds.events = POLLIN;

    ASSERT_GT(poll(&fds, 1, 0), 0);

    EXPECT_TRUE(fds.revents & POLLIN);
}

TEST_F(UdevWrapperTest, UdevMonitorFdIsUnreadableAfterProcessingEvents)
{
    mir::udev::Monitor monitor{mir::udev::Context()};

    monitor.enable();

    udev_environment.add_device("drm", "card0", NULL, {}, {});
    udev_environment.add_device("drm", "card1", NULL, {}, {});
    udev_environment.add_device("usb", "mightymouse", NULL, {}, {});

    struct pollfd fds;
    fds.fd = monitor.fd();
    fds.events = POLLIN;

    ASSERT_GT(poll(&fds, 1, 0), 0);
    ASSERT_TRUE(fds.revents & POLLIN);

    monitor.process_events([](mir::udev::Monitor::EventType, mir::udev::Device const&){});

    EXPECT_EQ(poll(&fds, 1, 0), 0);
}

TEST_F(UdevWrapperTest, UdevMonitorFiltersByPathAndType)
{
    mir::udev::Context ctx;

    mir::udev::Monitor monitor{mir::udev::Context()};
    bool event_received = false;

    monitor.filter_by_subsystem_and_type("drm", "drm_minor");

    monitor.enable();

    auto test_sysfspath = udev_environment.add_device("drm", "control64D", NULL, {}, {"DEVTYPE", "drm_minor"});
    auto minor_device = ctx.device_from_syspath(test_sysfspath);
    udev_environment.add_device("drm", "card0-LVDS1", test_sysfspath.c_str(), {}, {});
    udev_environment.add_device("usb", "mightymouse", NULL, {}, {});

    monitor.process_events([&event_received, minor_device]
        (mir::udev::Monitor::EventType, mir::udev::Device const& dev)
            {
                EXPECT_EQ(dev, *minor_device);
                event_received = true;
            });

    ASSERT_TRUE(event_received);
}

TEST_F(UdevWrapperTest, UdevMonitorFiltersAreAdditive)
{
    mir::udev::Context ctx;

    mir::udev::Monitor monitor{mir::udev::Context()};
    bool usb_event_received = false, drm_event_recieved = false;

    monitor.filter_by_subsystem_and_type("drm", "drm_minor");
    monitor.filter_by_subsystem_and_type("usb", "hid");

    monitor.enable();

    auto drm_sysfspath = udev_environment.add_device("drm", "control64D", NULL, {}, {"DEVTYPE", "drm_minor"});
    auto drm_device = ctx.device_from_syspath(drm_sysfspath);
    udev_environment.add_device("drm", "card0-LVDS1", drm_sysfspath.c_str(), {}, {});
    auto usb_sysfspath = udev_environment.add_device("usb", "mightymouse", NULL, {}, {"DEVTYPE", "hid"});
    auto usb_device = ctx.device_from_syspath(usb_sysfspath);

    monitor.process_events([&drm_event_recieved, drm_device, &usb_event_received, usb_device]
        (mir::udev::Monitor::EventType, mir::udev::Device const& dev)
            {
                if (dev == *drm_device)
                    drm_event_recieved = true;
                if (dev == *usb_device)
                    usb_event_received = true;
            });

    EXPECT_TRUE(drm_event_recieved);
    EXPECT_TRUE(usb_event_received);
}

TEST_F(UdevWrapperTest, UdevMonitorFiltersApplyAfterEnable)
{
    mir::udev::Context ctx;

    mir::udev::Monitor monitor{mir::udev::Context()};
    bool event_received = false;

    monitor.enable();

    monitor.filter_by_subsystem_and_type("drm", "drm_minor");

    auto test_sysfspath = udev_environment.add_device("drm", "control64D", NULL, {}, {"DEVTYPE", "drm_minor"});
    auto minor_device = ctx.device_from_syspath(test_sysfspath);
    udev_environment.add_device("drm", "card0-LVDS1", test_sysfspath.c_str(), {}, {});
    udev_environment.add_device("usb", "mightymouse", NULL, {}, {});

    monitor.process_events([&event_received, minor_device]
        (mir::udev::Monitor::EventType, mir::udev::Device const& dev)
            {
                EXPECT_EQ(dev, *minor_device);
                event_received = true;
            });

    ASSERT_TRUE(event_received);
}
