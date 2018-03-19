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
    MOCK_METHOD0(set_graphics_mode, void());
    MOCK_METHOD3(register_switch_handlers,
                 void(graphics::EventHandlerRegister&,
                      std::function<bool()> const&,
                      std::function<bool()> const&));
    MOCK_METHOD0(restore, void());
    MOCK_METHOD2(acquire_device_immediate, mir::Fd(int,int));

    boost::future<mir::Fd> acquire_device(int major, int minor)
    {
        try
        {
            return boost::make_ready_future(acquire_device_immediate(major, minor));
        }
        catch (...)
        {
            return boost::make_exceptional_future<mir::Fd>(std::current_exception());
        }
    }
};

}
}
}

#endif
