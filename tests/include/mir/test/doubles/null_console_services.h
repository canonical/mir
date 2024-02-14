/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_NULL_CONSOLE_SERVICES_H_
#define MIR_TEST_DOUBLES_NULL_CONSOLE_SERVICES_H_

#include "mir/fd.h"
#include "mir/console_services.h"

#include <boost/throw_exception.hpp>

namespace mir
{
namespace test
{
namespace doubles
{

class NullConsoleServices : public ConsoleServices
{
public:
    void register_switch_handlers(graphics::EventHandlerRegister&,
                                  std::function<bool()> const&,
                                  std::function<bool()> const&) override
    {
    }

    void restore() override {}

    std::unique_ptr<VTSwitcher> create_vt_switcher() override
    {
        BOOST_THROW_EXCEPTION((
            std::runtime_error{"NullConsoleServices does not implement VT switching"}));
    }

    std::future<std::unique_ptr<Device>> acquire_device(
        int, int,
        std::unique_ptr<Device::Observer> observer) override
    {
        std::promise<std::unique_ptr<Device>> null_promise;
        null_promise.set_value(nullptr);
        observer->activated(mir::Fd{});
        return null_promise.get_future();
    }

    void register_lock_handler(
        std::function<void()> const&,
        std::function<void()> const&) override
    {
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_CONSOLE_SERVICES_H_ */
