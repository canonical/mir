/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "utils.h"

namespace md = mir::dispatch;

md::FdEvents md::epoll_to_fd_event(epoll_event const& event)
{
    FdEvents val{0};
    if (event.events & EPOLLIN)
    {
        val |= FdEvent::readable;
    }
    if (event.events & EPOLLOUT)
    {
        val |= FdEvent::writable;
    }
    if (event.events & (EPOLLHUP | EPOLLRDHUP))
    {
        val |= FdEvent::remote_closed;
    }
    if (event.events & EPOLLERR)
    {
        val |= FdEvent::error;
    }
    return val;
}

int md::fd_event_to_epoll(FdEvents const& event)
{
    int epoll_value{0};
    if (event & FdEvent::readable)
    {
        epoll_value |= EPOLLIN;
    }
    if (event & FdEvent::writable)
    {
        epoll_value |= EPOLLOUT;
    }
    if (event & FdEvent::remote_closed)
    {
        epoll_value |= EPOLLRDHUP | EPOLLHUP;
    }
    if (event & FdEvent::error)
    {
        epoll_value |= EPOLLERR;
    }
    return epoll_value;
}
