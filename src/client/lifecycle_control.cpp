/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#include "lifecycle_control.h"

namespace mcl = mir::client;

mcl::LifecycleControl::LifecycleControl()
    : handle_lifecycle_event([](MirLifecycleState){})
{
}

mcl::LifecycleControl::~LifecycleControl()
{
}

void mcl::LifecycleControl::set_lifecycle_event_handler(std::function<void(MirLifecycleState)> const& fn)
{
    std::unique_lock<std::mutex> lk(guard);

    handle_lifecycle_event = fn;
}

void mcl::LifecycleControl::call_lifecycle_event_handler(uint32_t state)
{
    std::unique_lock<std::mutex> lk(guard);

    handle_lifecycle_event(static_cast<MirLifecycleState>(state));
}
