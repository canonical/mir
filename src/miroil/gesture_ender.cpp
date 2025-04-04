/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gesture_ender.h"

#include <mir/frontend/pointer_input_dispatcher.h>

namespace
{
struct PointerInputDispatcher : mir::frontend::PointerInputDispatcher
{
    void disable_dispatch_to_gesture_owner(std::function<void()> on_end_gesture) override;
};

std::mutex mutex;

std::weak_ptr<mir::frontend::PointerInputDispatcher> pointer_input_dispatcher;
std::function<void()> on_end_gesture = []{};


void PointerInputDispatcher::disable_dispatch_to_gesture_owner(std::function<void()> on_end_gesture)
{
    std::lock_guard lock{mutex};
    ::on_end_gesture = on_end_gesture;
}
}


void miroil::end_gesture()
{
    decltype(on_end_gesture) end_gesture = []{};

    {
        std::lock_guard lock{mutex};
        std::swap(on_end_gesture, end_gesture);
    }

    end_gesture();
}

auto miroil::the_pointer_input_dispatcher() -> std::shared_ptr<mir::frontend::PointerInputDispatcher>
{
    std::lock_guard lock{mutex};

    if (auto result = pointer_input_dispatcher.lock())
    {
        return result;
    }
    else
    {
        result = std::make_shared<PointerInputDispatcher>();
        pointer_input_dispatcher = result;
        return result;
    }
}
