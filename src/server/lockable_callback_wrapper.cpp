/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/lockable_callback_wrapper.h"

mir::LockableCallbackWrapper::LockableCallbackWrapper(
    std::shared_ptr<LockableCallback> const& wrapped,
    std::function<void()> const& precall_hook,
    std::function<void()> const& postcall_hook)
    : wrapped_callback{wrapped},
      precall_hook{precall_hook},
      postcall_hook{postcall_hook}
{
}

mir::LockableCallbackWrapper::LockableCallbackWrapper(
    std::shared_ptr<LockableCallback> const& wrapped,
    std::function<void()> const& precall_hook)
    : LockableCallbackWrapper(wrapped, precall_hook, []{})
{
}

void mir::LockableCallbackWrapper::operator()()
{
    precall_hook();
    (*wrapped_callback)();
    postcall_hook();
}

void mir::LockableCallbackWrapper::lock()
{
    wrapped_callback->lock();
}

void mir::LockableCallbackWrapper::unlock()
{
    wrapped_callback->unlock();
}

