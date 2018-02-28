/*
 * Copyright © 2018 Canonical Ltd.
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

#ifndef MIR_FRONTEND_EXECUTOR_H
#define MIR_FRONTEND_EXECUTOR_H

#include "mir/executor.h"

#include <wayland-server-core.h>

#include <memory>

namespace mir
{
namespace frontend
{
/**
 * Get an Executor which dispatches onto a wl_event_loop
 *
 * \note    The executor may outlive the wl_event_loop, but no tasks will be dispatched
 *          after the wl_event_loop is destroyed.
 *
 * \param [in]  loop    The event loop to dispatch on
 * \return              An Executor that queues onto the wl_event_loop
 */
std::shared_ptr<mir::Executor> executor_for_event_loop(wl_event_loop* loop);
}
}

#endif //MIR_FRONTEND_EXECUTOR_H
