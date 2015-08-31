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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/dispatch/readable_fd.h"

mir::dispatch::ReadableFd::ReadableFd(Fd fd, std::function<void()> const& on_readable) :
    fd{fd},
    readable{on_readable}
{
}

mir::Fd mir::dispatch::ReadableFd::watch_fd() const
{
    return fd;
}

bool mir::dispatch::ReadableFd::dispatch(FdEvents events)
{
    if (events & FdEvent::error)
        return false;

    if (events & FdEvent::readable)
        readable();

    return true;
}

mir::dispatch::FdEvents mir::dispatch::ReadableFd::relevant_events() const
{
    return FdEvent::readable;
}
