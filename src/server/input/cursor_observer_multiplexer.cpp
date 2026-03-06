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

#include <mir/input/cursor_observer_multiplexer.h>

namespace mi = mir::input;

mi::CursorObserverMultiplexer::CursorObserverMultiplexer(Executor& default_executor) :
    mir::ObserverMultiplexer<CursorObserver>{default_executor}
{
}

mi::CursorObserverMultiplexer::~CursorObserverMultiplexer() = default;

void mi::CursorObserverMultiplexer::register_interest(std::weak_ptr<CursorObserver> const& observer, Executor& executor)
{
    std::lock_guard lock{mutex};
    ObserverMultiplexer::register_interest(observer, executor);
    send_initial_state_locked(lock, observer);
}

void mi::CursorObserverMultiplexer::register_early_observer(std::weak_ptr<CursorObserver> const& observer, Executor& executor)
{
    std::lock_guard lock{mutex};
    ObserverMultiplexer::register_early_observer(observer, executor);
    send_initial_state_locked(lock, observer);
}

void mi::CursorObserverMultiplexer::send_initial_state_locked(std::lock_guard<std::mutex> const&, std::weak_ptr<CursorObserver> const& observer)
{
    auto const shared = observer.lock();
    if (shared)
    {
        if (pointer_is_usable)
            for_single_observer(*shared, &mi::CursorObserver::pointer_usable);
        else
            for_single_observer(*shared, &mi::CursorObserver::pointer_unusable);
        for_single_observer(*shared, &mi::CursorObserver::cursor_moved_to, last_abs_x, last_abs_y);
        for_single_observer(*shared, &mi::CursorObserver::image_set_to, cursor_image);
    }
}

void mi::CursorObserverMultiplexer::cursor_moved_to(float abs_x, float abs_y)
{
    std::lock_guard lock{mutex};
    last_abs_x = abs_x;
    last_abs_y = abs_y;
    for_each_observer(&CursorObserver::cursor_moved_to, abs_x, abs_y);
}

void mi::CursorObserverMultiplexer::pointer_usable()
{
    std::lock_guard lock{mutex};
    pointer_is_usable = true;
    for_each_observer(&CursorObserver::pointer_usable);
}

void mi::CursorObserverMultiplexer::pointer_unusable()
{
    std::lock_guard lock{mutex};
    pointer_is_usable = false;
    for_each_observer(&CursorObserver::pointer_unusable);
}

void mi::CursorObserverMultiplexer::image_set_to(std::shared_ptr<graphics::CursorImage> cursor)
{
    std::lock_guard lock{mutex};
    cursor_image = cursor;
    for_each_observer(&CursorObserver::image_set_to, std::move(cursor));
}
