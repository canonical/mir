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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "dispatchable.h"

#include <boost/throw_exception.hpp>
#include <X11/Xutil.h>
#include <inttypes.h>


namespace mi = mir::input;
namespace mix = mi::X;
namespace md = mir::dispatch;

mix::XDispatchable::XDispatchable(
    std::shared_ptr<::Display> const& conn,
    int raw_fd)
    : x11_connection(conn),
      fd(raw_fd),
      sink(nullptr),
      builder(nullptr)
{
}

mir::Fd mix::XDispatchable::watch_fd() const
{
    return fd;
}

bool mix::XDispatchable::dispatch(md::FdEvents events)
{
    auto ret = true;

    if (events & (md::FdEvent::remote_closed | md::FdEvent::error))
        ret = false;

    if (events & md::FdEvent::readable)
    {
            }

    return ret;
}

md::FdEvents mix::XDispatchable::relevant_events() const
{
    return md::FdEvent::readable | md::FdEvent::error | md::FdEvent::remote_closed;
}

}
