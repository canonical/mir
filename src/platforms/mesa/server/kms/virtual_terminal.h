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

#ifndef MIR_GRAPHICS_MESA_VIRTUAL_TERMINAL_H_
#define MIR_GRAPHICS_MESA_VIRTUAL_TERMINAL_H_

#include <functional>
#include <boost/thread/future.hpp>

namespace mir
{
class Fd;

namespace graphics
{
class EventHandlerRegister;

namespace mesa
{

class VirtualTerminal
{
public:
    virtual ~VirtualTerminal() = default;

    virtual void set_graphics_mode() = 0;
    virtual void register_switch_handlers(
        EventHandlerRegister& handlers,
        std::function<bool()> const& switch_away,
        std::function<bool()> const& switch_back) = 0;
    virtual void restore() = 0;

    /**
     * Asynchronously acquire access to a device node
     *
     * \param major             [in] major number of requested device node
     * \param minor             [in] minor number of requested device node
     */
    virtual boost::unique_future<Fd> acquire_device(int major, int minor) = 0;

protected:
    VirtualTerminal() = default;
    VirtualTerminal(VirtualTerminal const&) = delete;
    VirtualTerminal& operator=(VirtualTerminal const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_VIRTUAL_TERMINAL_H_ */
