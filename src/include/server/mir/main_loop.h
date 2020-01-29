/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#ifndef MIR_MAIN_LOOP_H_
#define MIR_MAIN_LOOP_H_

#include "mir/graphics/event_handler_register.h"
#include "mir/time/alarm_factory.h"
#include "mir/server_action_queue.h"
#include "mir/executor.h"

namespace mir
{

class MainLoop :
    public graphics::EventHandlerRegister,
    public time::AlarmFactory,
    public ServerActionQueue,
    public Executor
{
public:
    virtual void run() = 0;
    virtual void stop() = 0;
    virtual bool running() const = 0;
};

}

#endif /* MIR_MAIN_LOOP_H_ */
