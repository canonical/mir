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

#include "X_dispatchable.h"
#include "../../debug.h"

namespace mi = mir::input;
namespace mix = mi::X;

mir::Fd mix::XDispatchable::watch_fd() const
{
    CALLED
    return mir::Fd{0};
}

bool mix::XDispatchable::dispatch(dispatch::FdEvents /*events*/)
{
    CALLED
    return false;
}

mir::dispatch::FdEvents mix::XDispatchable::relevant_events() const
{
    CALLED
    return dispatch::FdEvent::readable;
}
