/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_DISPATCH_ACTION_QUEUE_H_
#define MIR_DISPATCH_ACTION_QUEUE_H_

#include "mir/fd.h"
#include "mir/dispatch/dispatchable.h"

#include <list>
#include <mutex>

namespace mir
{
namespace dispatch
{

class ActionQueue : public Dispatchable
{
public:
    ActionQueue();
    Fd watch_fd() const override;

    void enqueue(std::function<void()> const& action);

    bool dispatch(FdEvents events) override;
    FdEvents relevant_events() const override;
private:
    bool consume();
    void wake();
    mir::Fd event_fd;
    std::mutex list_lock;
    std::list<std::function<void()>> actions;
};
}
}

#endif // MIR_DISPATCH_ACTION_QUEUE_H_
