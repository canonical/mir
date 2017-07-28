/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef MIR_CLIENT_EVENT_HANDLER_REGISTER_H_
#define MIR_CLIENT_EVENT_HANDLER_REGISTER_H_

#include "mir_toolkit/event.h"

#include <functional>

namespace mir
{
namespace client
{

class EventHandlerRegister
{
public:
    virtual ~EventHandlerRegister() = default;

    virtual int register_event_handler(std::function<void(MirEvent const&)> const&) = 0;
    virtual void unregister_event_handler(int id) = 0;

protected:
    EventHandlerRegister() = default;
    EventHandlerRegister(EventHandlerRegister const&) = delete;
    EventHandlerRegister& operator=(EventHandlerRegister const&) = delete;
};
} // namespace client
} // namespace mir

#endif // MIR_CLIENT_EVENT_HANDLER_REGISTER_H_
