/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/input/cursor_observer_multiplexer.h"

namespace mi = mir::input;

mi::CursorObserverMultiplexer::CursorObserverMultiplexer(Executor& default_executor) :
    mir::ObserverMultiplexer<CursorObserver>{default_executor}
{
}

mi::CursorObserverMultiplexer::~CursorObserverMultiplexer() = default;

void mi::CursorObserverMultiplexer::cursor_moved_to(float abs_x, float abs_y)
{
    for_each_observer(&CursorObserver::cursor_moved_to, abs_x, abs_y);
}

void mi::CursorObserverMultiplexer::pointer_usable()
{
    for_each_observer(&CursorObserver::pointer_usable);
}

void mi::CursorObserverMultiplexer::pointer_unusable()
{
    for_each_observer(&CursorObserver::pointer_unusable);
}
