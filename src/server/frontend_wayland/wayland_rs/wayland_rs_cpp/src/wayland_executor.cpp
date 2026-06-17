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

#include "wayland_executor.h"

#include "wayland_rs/src/ffi.rs.h"

namespace mrs = mir::wayland_rs;

mrs::WaylandExecutor::WaylandExecutor(mrs::WaylandServer& server)
    : server{server}
{
}

void mrs::WaylandExecutor::spawn(std::function<void()>&& work)
{
    bool need_signal;
    {
        std::lock_guard<std::mutex> lock{mutex};
        pending_work.push_back(std::move(work));

        // Only raise a signal if one is not already outstanding; any work
        // appended while a signal is pending (or a drain is in progress) is
        // picked up by that drain, so one signal per batch suffices.
        need_signal = !signal_pending;
        if (need_signal)
            signal_pending = true;
    }

    // If the signal was dropped because the server is not yet running, clear
    // the flag so that the next spawn re-signals rather than stranding the
    // queue.
    if (need_signal && !server.schedule_work())
    {
        std::lock_guard<std::mutex> lock{mutex};
        signal_pending = false;
    }
}

auto mrs::WaylandExecutor::execute() -> void
{
    std::vector<std::function<void()>> work;
    {
        std::lock_guard<std::mutex> lock{mutex};
        work.swap(pending_work);
        signal_pending = false;
    }

    // Run the work outside the lock so that it may itself schedule more work.
    for (auto& task : work)
    {
        if (task)
            task();
    }
}
