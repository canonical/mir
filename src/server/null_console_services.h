/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_NULL_CONSOLE_SERVICES_H_
#define MIR_NULL_CONSOLE_SERVICES_H_

#include "mir/console_services.h"

namespace mir
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

    boost::future<Fd> acquire_device(int, int) override
    {
        return boost::make_ready_future<Fd>(-1);
    }
};

}

#endif /* MIR_NULL_CONSOLE_SERVICES_H_ */
