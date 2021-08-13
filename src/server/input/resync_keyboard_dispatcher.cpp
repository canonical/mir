/*
 * Copyright © 2021 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "resync_keyboard_dispatcher.h"

#include "mir/input/seat.h"
#include "mir/events/resync_keyboard_event.h"

#include <unordered_set>

namespace mi = mir::input;

mi::ResyncKeyboardDispatcher::ResyncKeyboardDispatcher(std::shared_ptr<InputDispatcher> const& next_dispatcher)
    : next_dispatcher{next_dispatcher}
{
}

bool mi::ResyncKeyboardDispatcher::dispatch(std::shared_ptr<MirEvent const> const& event)
{
    return next_dispatcher->dispatch(event);
}

void mi::ResyncKeyboardDispatcher::start()
{
    next_dispatcher->start();
    auto const ev = std::make_shared<MirResyncKeyboardEvent>();
    next_dispatcher->dispatch(ev);
}

void mi::ResyncKeyboardDispatcher::stop()
{
    next_dispatcher->stop();
}
