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

#ifndef MIR_DISPATCH_UTILS_H_
#define MIR_DISPATCH_UTILS_H_

#include "mir/dispatch/dispatchable.h"
#include <sys/epoll.h>

namespace mir
{
namespace dispatch
{

FdEvents epoll_to_fd_event(epoll_event const& event);

int fd_event_to_epoll(FdEvents const& event);

}
}


#endif // MIR_DISPATCH_UTILS_H_
