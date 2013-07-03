/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

#include "src/server/graphics/gbm/udev_wrapper.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

#include <umockdev.h>
#include <libudev.h>

namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace mtf=mir::mir_test_framework;

class UdevWrapperTest : public ::testing::Test
{
public:
    mtf::UdevEnvironment udev_environment;
};

TEST_F(UdevWrapperTest, IteratesOverCorrectNumberOfDevices)
{
    udev_environment.add_device("drm", "fakedev1", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev2", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev3", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev4", NULL, {}, {});
    udev_environment.add_device("drm", "fakedev5", NULL, {}, {});

    udev* ctx = udev_new();
    mgg::UdevEnumerator enumerator(ctx);

    enumerator.scan_devices();

    int device_count = 0;
    for (auto& device : enumerator)
    {
        (void)device;
        ++device_count;
    }

    ASSERT_EQ(device_count, 5);

    udev_unref(ctx);
}

TEST_F(UdevWrapperTest, EnumeratorMatchSubsystemIncludesCorrectDevices)
{
    udev_environment.add_device("drm", "fakedrm1", NULL, {}, {});
    udev_environment.add_device("scsi", "fakescsi1", NULL, {}, {});
    udev_environment.add_device("drm", "fakedrm2", NULL, {}, {});
    udev_environment.add_device("usb", "fakeusb1", NULL, {}, {});
    udev_environment.add_device("usb", "fakeusb2", NULL, {}, {});

    udev* ctx = udev_new();
    mgg::UdevEnumerator devices(ctx);

    devices.add_match_subsystem("drm");
    devices.scan_devices();
    for (auto& device : devices)
    {
        ASSERT_STREQ("drm", device.subsystem());
    }

    udev_unref(ctx);
}
