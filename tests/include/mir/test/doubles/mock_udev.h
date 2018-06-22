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

#ifndef MIR_TEST_DOUBLES_MOCK_UDEV_H_
#define MIR_TEST_DOUBLES_MOCK_UDEV_H_

#include <gmock/gmock.h>

#include <libudev.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockUdev
{
public:
    MockUdev();
    ~MockUdev() noexcept;
    MOCK_METHOD1(udev_device_get_devnum, dev_t(udev_device*));
    MOCK_METHOD1(udev_device_get_devnode, char const*(udev_device*));
    MOCK_METHOD2(udev_device_get_property_value, char const*(udev_device* device, char const* property));
    MOCK_METHOD1(udev_device_unref, udev_device*(udev_device* device));
    MOCK_METHOD1(udev_device_ref,udev_device*(udev_device* device));
};

}
}
}
#endif
