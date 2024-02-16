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

#include "session_locker.h"
#include "mir/frontend/surface_stack.h"
#include "mir/executor.h"

namespace ms = mir::scene;
namespace mf = mir::frontend;

ms::SessionLocker::SessionLocker(
    std::shared_ptr<Executor> const& executor,
    std::shared_ptr<mf::SurfaceStack> const& surface_stack)
    : mf::SessionLocker(*executor),
      executor{executor},
      surface_stack{surface_stack}
{
}

void ms::SessionLocker::on_lock()
{
    if (!surface_stack->screen_is_locked())
    {
        screen_lock_handle = surface_stack->lock_screen();

        for_each_observer(&SessionLockObserver::on_lock);
    }
}

void ms::SessionLocker::on_unlock()
{
    if (surface_stack->screen_is_locked())
    {
        screen_lock_handle->allow_to_be_dropped();
        screen_lock_handle = nullptr;
        for_each_observer(&SessionLockObserver::on_unlock);
    }
}
