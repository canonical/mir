/*
 * Copyright © Canonical Ltd.
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
    MOCK_METHOD(char const*, subsystem, (), (const override));
    MOCK_METHOD(char const*, devtype, (), (const override));
    MOCK_METHOD(char const*, devpath, (), (const override));
    MOCK_METHOD(char const*, devnode, (), (const override));
    MOCK_METHOD(char const*, property, (char const*), (const override));
    MOCK_METHOD(dev_t, devnum, (), (const override));
    MOCK_METHOD(char const*, sysname, (), (const override));
    MOCK_METHOD(bool, initialised, (), (const override));
    MOCK_METHOD(char const*, syspath, (), (const override));
    MOCK_METHOD(std::shared_ptr<udev_device>, as_raw, (), (const override));
    MOCK_METHOD(char const*, driver, (), (const override));
    MOCK_METHOD(std::unique_ptr<mir::udev::Device>, parent, (), (const override));
    MOCK_METHOD(std::unique_ptr<mir::udev::Device>, clone, (), (const override));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_UDEV_DEVICE_H_
