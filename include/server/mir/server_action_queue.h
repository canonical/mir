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

#ifndef MIR_SERVER_ACTION_QUEUE_H_
#define MIR_SERVER_ACTION_QUEUE_H_

#include <functional>

namespace mir
{

typedef std::function<void()> ServerAction;

class ServerActionQueue
{
public:
    virtual ~ServerActionQueue() = default;

    virtual void enqueue(void const* owner, ServerAction const& action) = 0;
    /**
     * Enqueue an action to be run, guaranteeing that it *will* be run.
     *
     * The action will be run even if the queue is not normally being drained
     * (for example, if the main loop is suspended). If running on the queue is
     * not possible, the action will be run on the caller's thread.
     *
     * \param [in]  action  Functor to invoke.
     */
    virtual void enqueue_with_guaranteed_execution(ServerAction const& action) = 0;
    virtual void pause_processing_for(void const* owner) = 0;
    virtual void resume_processing_for(void const* owner) = 0;

protected:
    ServerActionQueue() = default;
    ServerActionQueue(ServerActionQueue const&) = delete;
    ServerActionQueue& operator=(ServerActionQueue const&) = delete;
};

}

#endif /* MIR_SERVER_ACTION_QUEUE_H_ */
