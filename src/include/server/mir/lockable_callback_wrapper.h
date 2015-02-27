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

#ifndef MIR_LOCKABLE_CALLBACK_WRAPPER_H_
#define MIR_LOCKABLE_CALLBACK_WRAPPER_H_

#include "mir/lockable_callback.h"

#include <functional>
#include <memory>

namespace mir
{
class LockableCallbackWrapper : public LockableCallback
{
public:
    LockableCallbackWrapper(std::shared_ptr<LockableCallback> const& wrapped,
                            std::function<void()> const& precall_hook);

    LockableCallbackWrapper(std::shared_ptr<LockableCallback> const& wrapped,
                            std::function<void()> const& precall_hook,
                            std::function<void()> const& postcall_hook);

    void operator()() override;
    void lock() override;
    void unlock() override;

private:
    std::shared_ptr<LockableCallback> wrapped_callback;
    std::function<void()> precall_hook;
    std::function<void()> postcall_hook;
};

}

#endif
