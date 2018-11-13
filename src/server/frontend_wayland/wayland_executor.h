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
#include "mir/fd.h"

#include <wayland-server-core.h>

#include <mutex>
#include <memory>
#include <deque>

namespace mir
{
namespace frontend
{
class WaylandExecutor : public Executor
{
public:
    explicit WaylandExecutor(wl_event_loop* loop);
    ~WaylandExecutor();

    void spawn(std::function<void()>&& work) override;

private:
    std::function<void()> get_work();
    static int on_notify(int fd, uint32_t, void* data);

    mir::Fd const notify_fd;
    std::mutex mutex;
    std::deque<std::function<void()>> workqueue;
};
}
}

#endif //MIR_FRONTEND_EXECUTOR_H
