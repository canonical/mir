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
 * Authored by: Christopher Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_UDEV_DEVICE_H_
#define MIR_TEST_DOUBLES_MOCK_UDEV_DEVICE_H_

#include "mir/udev/wrapper.h"

#include <gmock/gmock.h>


namespace mir
{
namespace test
{
namespace doubles
{

class MockUdevDevice : public mir::udev::Device
{
public:
    MOCK_CONST_METHOD0(subsystem, char const*(void));
    MOCK_CONST_METHOD0(devtype, char const*(void));
    MOCK_CONST_METHOD0(devpath, char const*(void));
    MOCK_CONST_METHOD0(devnode, char const*(void));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_UDEV_DEVICE_H_
