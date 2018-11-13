/*
 * Copyright © 2015-2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wayland_executor.h"

#include "mir/fd.h"
#include "mir/log.h"

#include <sys/eventfd.h>

#include <boost/throw_exception.hpp>

#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <system_error>

namespace mf = mir::frontend;

mf::WaylandExecutor::WaylandExecutor(wl_display* display)
    : notify_fd{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK)}
{
    if (notify_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to create IPC pause notification eventfd"}));
    }
    if (!display)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Invalid wl_display"}));
    }
    wl_event_loop_add_fd(
        wl_display_get_event_loop(display),
        notify_fd,
        WL_EVENT_READABLE,
        &WaylandExecutor::on_notify,
        this);
}

mf::WaylandExecutor::~WaylandExecutor() = default;

void mf::WaylandExecutor::spawn (std::function<void ()>&& work)
{
    {
        std::lock_guard<std::mutex> lock{mutex};
        workqueue.emplace_back(std::move(work));
    }
    if (auto err = eventfd_write(notify_fd, 1))
    {
        BOOST_THROW_EXCEPTION((std::system_error{err, std::system_category(), "eventfd_write failed to notify event loop"}));
    }
}

std::function<void()> mf::WaylandExecutor::get_work()
{
    std::lock_guard<std::mutex> lock{mutex};
    if (!workqueue.empty())
    {
        auto const work = std::move(workqueue.front());
        workqueue.pop_front();
        return work;
    }
    return {};
}

int mf::WaylandExecutor::on_notify(int fd, uint32_t, void* data)
{
    auto executor = static_cast<WaylandExecutor*>(data);

    eventfd_t unused;
    if (auto err = eventfd_read(fd, &unused))
    {
        mir::log_error(
            "eventfd_read failed to consume wakeup notification: %s (%i)",
            strerror(err),
            err);
    }

    while (auto work = executor->get_work())
    {
        try
        {
            work();
        }
        catch(...)
        {
            mir::log(
                mir::logging::Severity::critical,
                MIR_LOG_COMPONENT,
                std::current_exception(),
                "Exception processing Wayland event loop work item");
        }
    }

    return 0;
}

