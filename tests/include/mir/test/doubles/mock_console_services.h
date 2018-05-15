/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_CONSOLE_SERVICES_H_
#define MIR_TEST_DOUBLES_MOCK_CONSOLE_SERVICES_H_

#include "mir/console_services.h"

#include "mir/test/gmock_fixes.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockConsoleServices : public ConsoleServices
{
public:
    MOCK_METHOD3(register_switch_handlers,
                 void(graphics::EventHandlerRegister&,
                      std::function<bool()> const&,
                      std::function<bool()> const&));
    MOCK_METHOD0(restore, void());
    MOCK_METHOD3(
        acquire_device_immediate,
        std::unique_ptr<Device>(
            int,int,
            Device::Observer*));

    std::future<std::unique_ptr<Device>> acquire_device(
        int major, int minor,
        std::unique_ptr<Device::Observer> observer  )
    {
        std::promise<std::unique_ptr<Device>> promise;
        try
        {
            promise.set_value(
                acquire_device_immediate(
                    major, minor,
                    observer.get()));
        }
        catch (...)
        {
            promise.set_exception(std::current_exception());
        }
        return promise.get_future();
    }
};

}
}
}

#endif
