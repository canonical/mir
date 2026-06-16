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
    std::uint32_t work_id;
    {
        std::lock_guard<std::mutex> lock{mutex};
        work_id = next_id++;
        pending_work.emplace(work_id, std::move(work));
    }

    server.schedule_work(work_id);
}

auto mrs::WaylandExecutor::execute(std::uint32_t work_id) -> void
{
    std::function<void()> work;
    {
        std::lock_guard<std::mutex> lock{mutex};
        auto const it = pending_work.find(work_id);
        if (it == pending_work.end())
            return;

        work = std::move(it->second);
        pending_work.erase(it);
    }

    // Run the work outside the lock so that it may itself schedule more work.
    if (work)
        work();
}
