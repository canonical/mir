/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/test/doubles/triggered_main_loop.h"
#include "mir/test/doubles/stub_alarm.h"

#include <algorithm>
#include <sys/select.h>

namespace mtd = mir::test::doubles;

using base = ::testing::NiceMock<mtd::MockMainLoop>;
void mtd::TriggeredMainLoop::register_fd_handler(std::initializer_list<int> fds, void const* owner, fd_callback const& handler)
{
    base::register_fd_handler(fds, owner, handler);
    for (int fd : fds)
    {
        fd_callbacks.emplace_back(Item{fd, owner, handler});
    }
}

void mtd::TriggeredMainLoop::unregister_fd_handler(void const* owner)
{
    base::unregister_fd_handler(owner);
    fd_callbacks.erase(
        remove_if(
            begin(fd_callbacks),
            end(fd_callbacks),
            [owner](Item const& item)
            {
                return item.owner == owner;
            }),
        end(fd_callbacks)
        );
}

void mtd::TriggeredMainLoop::trigger_pending_fds()
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int max_fd = 0;

    for (auto const & item : fd_callbacks)
    {
        FD_SET(item.fd, &read_fds);
        max_fd = std::max(item.fd, max_fd);
    }

    struct timeval do_not_wait{0, 0};

    if (select(max_fd+1, &read_fds, nullptr, nullptr, &do_not_wait))
    {
        for (auto const& item : fd_callbacks)
        {
            if (FD_ISSET(item.fd, &read_fds))
            {
                item.callback(item.fd);
            }
        }
    }
}

void mtd::TriggeredMainLoop::enqueue(void const* owner, ServerAction const& action)
{
    base::enqueue(owner, action);
    actions.push_back(action);
}

void mtd::TriggeredMainLoop::enqueue_with_guaranteed_execution (ServerAction const& action)
{
    base::enqueue(nullptr, action);
    actions.push_back(action);
}

void mtd::TriggeredMainLoop::trigger_server_actions()
{
    for (auto const& action : actions)
        action();
    actions.clear();
}

std::unique_ptr<mir::time::Alarm>
mtd::TriggeredMainLoop::create_alarm(callback const& call)
{
    base::create_alarm(call);
    timeout_callbacks.push_back(call);
    return std::unique_ptr<mir::time::Alarm>{new mtd::StubAlarm};
}

void mtd::TriggeredMainLoop::fire_all_alarms()
{
    for(auto const& callback : timeout_callbacks)
        callback();
}

void mtd::TriggeredMainLoop::spawn(std::function<void()>&& work)
{
    base::spawn(std::function<void()>{work});
    this->work.emplace_back(std::move(work));
}

void mtd::TriggeredMainLoop::trigger_spawned_work()
{
    for (auto const& action : work)
    {
        action();
    }
    work.clear();
}
