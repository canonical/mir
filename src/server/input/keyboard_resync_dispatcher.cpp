/*
 * Copyright © Canonical Ltd.
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
 */

#include "keyboard_resync_dispatcher.h"

#include <mir/events/keyboard_resync_event.h>

namespace mi = mir::input;

mi::KeyboardResyncDispatcher::KeyboardResyncDispatcher(std::shared_ptr<InputDispatcher> const& next_dispatcher)
    : next_dispatcher{next_dispatcher}
{
}

bool mi::KeyboardResyncDispatcher::dispatch(std::shared_ptr<MirEvent const> const& event)
{
    return next_dispatcher->dispatch(event);
}

void mi::KeyboardResyncDispatcher::start()
{
    next_dispatcher->start();
    auto const ev = std::make_shared<MirKeyboardResyncEvent>();
    next_dispatcher->dispatch(ev);
}

void mi::KeyboardResyncDispatcher::stop()
{
    next_dispatcher->stop();
}
