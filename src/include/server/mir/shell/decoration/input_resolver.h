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

#ifndef MIR_SHELL_DECORATION_INPUT_MANAGER_H_
#define MIR_SHELL_DECORATION_INPUT_MANAGER_H_

#include "mir/geometry/rectangle.h"
#include "mir/scene/null_surface_observer.h"
#include "mir_toolkit/common.h"

#include <memory>
#include <mutex>
#include <map>
#include <optional>

struct MirEvent;

namespace mir
{
namespace scene
{
class Surface;
}
namespace input
{
class CursorImages;
}
namespace shell
{
class Shell;
namespace decoration
{
class Decoration;
class WindowState;
class BasicDecoration;
class StaticGeometry;
template<typename T> class ThreadsafeAccess;

enum class ButtonState
{
    Up,         ///< The user is not interacting with this button
    Hovered,    ///< The user is hovering over this button
    Down,       ///< The user is currently pressing this button
};

/// Pointer or touchpoint
struct DeviceEvent
{
    DeviceEvent(geometry::Point location, bool pressed)
        : location{location},
          pressed{pressed}

    {
    }

    geometry::Point location;
    bool pressed;
};

// Given an input event, figures out which decoration callback should be
// invoked to respond.
class InputResolver
{
public:


    InputResolver(std::shared_ptr<mir::scene::Surface> const& decoration_surface);
    virtual ~InputResolver();

    auto latest_event() -> std::shared_ptr<MirEvent const> const;

protected:
    // To be overriden by child classes.
    // Called into from {pointer,touch}_{event,leave}

    /// The input device has entered the surface
    virtual void process_enter(DeviceEvent& device) = 0;
    /// The input device has left the surface
    virtual void process_leave() = 0;
    /// The input device has clicked down
    /// A touch triggers a process_enter() followed by a process_down()
    virtual void process_down() = 0;
    /// The input device has released
    /// A touch release triggers a process_up() followed by a process_leave()
    virtual void process_up() = 0;
    /// The device has moved while up
    virtual void process_move(DeviceEvent& device) = 0;
    /// The device has moved while down
    virtual void process_drag(DeviceEvent& device) = 0;

    std::mutex mutex;
private:
    struct Observer;
    friend Observer;

    void pointer_event(std::shared_ptr<MirEvent const> const& event, geometry::Point location, bool pressed);
    void pointer_leave(std::shared_ptr<MirEvent const> const& event);
    void touch_event(std::shared_ptr<MirEvent const> const& event, int32_t id, geometry::Point location);
    void touch_up(std::shared_ptr<MirEvent const> const& event, int32_t id);

    /// The most recent event, or the event currently being processed
    std::shared_ptr<MirEvent const> latest_event_;
    std::optional<DeviceEvent> pointer;
    std::map<int32_t, DeviceEvent> touches;
    std::shared_ptr<Observer> input_observer;
    std::shared_ptr<scene::Surface> decoration_surface;
};
}
}
}

#endif
