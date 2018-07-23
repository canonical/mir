/*
 * Copyright © 2016 Canonical Ltd.
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

#include "mir/test/doubles/mock_udev.h"

namespace mtd = mir::test::doubles;

namespace
{
mtd::MockUdev* global_udev = nullptr;
}

mtd::MockUdev::MockUdev()
{
    using namespace testing;
    assert(global_udev == nullptr && "Only one udev mock object per process is allowed");
    global_udev = this;

    ON_CALL(*this, udev_device_ref(_))
        .WillByDefault(ReturnArg<0>());
}

mtd::MockUdev::~MockUdev() noexcept
{
    global_udev = nullptr;
}

dev_t udev_device_get_devnum(udev_device* dev)
{
    return global_udev->udev_device_get_devnum(dev);
}

char const* udev_device_get_devnode(udev_device* dev)
{
    return global_udev->udev_device_get_devnode(dev);
}

char const* udev_device_get_property_value(udev_device* dev, char const* property)
{
    return global_udev->udev_device_get_property_value(dev, property);
}

udev_device* udev_device_unref(udev_device* device)
{
    return global_udev->udev_device_unref(device);
}

udev_device* udev_device_ref(udev_device* device)
{
    return global_udev->udev_device_ref(device);
}
