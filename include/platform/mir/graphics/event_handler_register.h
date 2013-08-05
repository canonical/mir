/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_GRAPHICS_EVENT_HANDLER_REGISTER_H_
#define MIR_GRAPHICS_EVENT_HANDLER_REGISTER_H_

#include <functional>
#include <initializer_list>

namespace mir
{

namespace graphics
{
class EventHandlerRegister
{
public:
    virtual void register_signal_handler(
        std::initializer_list<int> signals,
        std::function<void(int)> const& handler) = 0;

    virtual void register_fd_handler(
        std::initializer_list<int> fds,
        std::function<void(int)> const& handler) = 0;

protected:
    EventHandlerRegister() = default;
    virtual ~EventHandlerRegister() = default;
    EventHandlerRegister(EventHandlerRegister const&) = delete;
    EventHandlerRegister& operator=(EventHandlerRegister const&) = delete;
};
}
}


#endif /* MIR_GRAPHICS_EVENT_HANDLER_REGISTER_H_ */
