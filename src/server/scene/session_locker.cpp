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
#include "mir/console_services.h"

#include <algorithm>

namespace ms = mir::scene;

ms::SessionLocker::SessionLocker(
    std::shared_ptr<mir::ConsoleServices> const& console_services,
    std::shared_ptr<mf::SurfaceStack> const& surface_stack)
    : surface_stack{surface_stack}
{
    console_services->register_lock_handler(
        [&] { request_lock(); },
        [&] { request_unlock(); });
}

void ms::SessionLocker::add_observer(std::shared_ptr<mf::SessionLockObserver> const& observer)
{
    observers.push_back(observer);
}

void ms::SessionLocker::remove_observer(std::weak_ptr<mf::SessionLockObserver> const& observer)
{
    observers.erase(std::remove(observers.begin(), observers.end(), observer.lock()));
}

void ms::SessionLocker::request_lock()
{
    if (!surface_stack->screen_is_locked())
    {
        screen_lock_handle = surface_stack->lock_screen();
        for (auto const& observer : observers)
            observer->on_lock();
    }
}

void ms::SessionLocker::request_unlock()
{
    if (surface_stack->screen_is_locked())
    {
        screen_lock_handle->allow_to_be_dropped();
        screen_lock_handle = nullptr;
        for (auto const& observer : observers)
            observer->on_unlock();
    }
}
