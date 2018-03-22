/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_PLATFORM_CONSOLE_SERVICES_H_
#define MIR_PLATFORM_CONSOLE_SERVICES_H_

#include <functional>

#define BOOST_THREAD_PROVIDES_FUTURE
#include <boost/thread/future.hpp>

namespace mir
{
class Fd;

namespace graphics
{
class EventHandlerRegister;
}

class ConsoleServices
{
public:
    virtual ~ConsoleServices() = default;

    virtual void register_switch_handlers(
        graphics::EventHandlerRegister& handlers,
        std::function<bool()> const& switch_away,
        std::function<bool()> const& switch_back) = 0;
    virtual void restore() = 0;

    /**
     * Asynchronously acquire access to a device node
     *
     * \param major             [in] major number of requested device node
     * \param minor             [in] minor number of requested device node
     */
    virtual boost::future<Fd> acquire_device(int major, int minor) = 0;

protected:
    ConsoleServices() = default;
    ConsoleServices(ConsoleServices const&) = delete;
    ConsoleServices& operator=(ConsoleServices const&) = delete;
};

}

#endif /* MIR_PLATFORM_CONSOLE_SERVICES_H_*/
