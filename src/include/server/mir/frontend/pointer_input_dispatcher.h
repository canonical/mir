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

#ifndef MIR_FRONTEND_POINTER_INPUT_DISPATCHER_H_
#define MIR_FRONTEND_POINTER_INPUT_DISPATCHER_H_

#include <functional>
#include <memory>

namespace mir
{
namespace scene { class Surface; }

namespace frontend
{

/// An interface to modify the routing of pointer events
class PointerInputDispatcher
{
public:
    virtual ~PointerInputDispatcher() = default;

    // Sometimes (e.g. drag-n-drop) the frontend wants routing to ignore the surface on which gestures start
    virtual void disable_dispatch_to_gesture_owner(std::function<void()> on_end_gesture) = 0;

protected:
    PointerInputDispatcher() = default;
    PointerInputDispatcher(PointerInputDispatcher const&) = delete;
    PointerInputDispatcher& operator=(PointerInputDispatcher const&) = delete;
};

}
}

#endif // MIR_FRONTEND_POINTER_INPUT_DISPATCHER_H_
