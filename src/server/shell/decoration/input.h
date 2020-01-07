/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_INPUT_H_
#define MIR_SHELL_DECORATION_INPUT_H_

#include "mir/geometry/rectangle.h"
#include "mir_toolkit/common.h"

#include <memory>
#include <vector>
#include <mutex>
#include <functional>
#include <map>
#include <experimental/optional>

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

enum class ButtonFunction
{
    Close,
    Maximize,
    Minimize,
};

struct ButtonInfo
{
    ButtonFunction function;
    ButtonState state;
    geometry::Rectangle rect;

    auto operator==(ButtonInfo const& other) const -> bool
    {
        return function == other.function &&
               state == other.state &&
               rect == other.rect;
    }
};

/// Describes the state of the interface (what buttons are pushed, etc)
class InputState
{
public:
    InputState(
        std::vector<ButtonInfo> const& buttons,
        std::vector<geometry::Rectangle> const& input_shape)
        : buttons_{buttons},
          input_shape_{input_shape}
    {
    }

    auto buttons() const -> std::vector<ButtonInfo> const& { return buttons_; }
    auto input_shape() const -> std::vector<geometry::Rectangle> const& { return input_shape_; }

private:
    InputState(InputState const&) = delete;
    InputState& operator=(InputState const&) = delete;

    std::vector<ButtonInfo> const buttons_;
    std::vector<geometry::Rectangle> const input_shape_;
};

/// Manages the observer that listens to user input
class InputManager
{
public:
    InputManager(
        std::shared_ptr<StaticGeometry const> const& static_geometry,
        std::shared_ptr<scene::Surface> const& decoration_surface,
        WindowState const& window_state,
        std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const& decoration);
    ~InputManager();

    void update_window_state(WindowState const& window_state);
    auto state() -> std::unique_ptr<InputState>;

private:
    static std::chrono::nanoseconds const double_click_threshold;

    InputManager(InputManager const&) = delete;
    InputManager& operator=(InputManager const&) = delete;

    static auto resize_edge_rect(
        WindowState const& window_state,
        StaticGeometry const& static_geometry,
        MirResizeEdge resize_edge) -> geometry::Rectangle;
    void pointer_event(std::chrono::nanoseconds timestamp, geometry::Point location, bool pressed);
    void pointer_leave(std::chrono::nanoseconds timestamp);
    void touch_event(int32_t id, std::chrono::nanoseconds timestamp, geometry::Point location);
    void touch_up(int32_t id, std::chrono::nanoseconds timestamp);

    class Observer;

    std::mutex mutex;
    std::shared_ptr<StaticGeometry const> const static_geometry;
    std::shared_ptr<scene::Surface> decoration_surface;
    std::shared_ptr<Observer> const observer;
    std::shared_ptr<ThreadsafeAccess<BasicDecoration>> decoration;
    std::vector<geometry::Rectangle> input_shape;
    /// Timestamp of the most recent event, or the event currently being processed
    std::chrono::nanoseconds event_timestamp{0};
    /// Timestamp of the previous up event (not the one currently being processed)
    std::chrono::nanoseconds previous_up_timestamp{0};
    std::string current_cursor_name{""};

    struct Widget
    {
        Widget(ButtonFunction button)
            : button{button}
        {
        }

        Widget(MirResizeEdge resize_edge)
            : resize_edge{resize_edge}
        {
        }

        geometry::Rectangle rect;
        ButtonState state{ButtonState::Up};
        std::experimental::optional<ButtonFunction> const button;
        // mir_resize_edge_none is used to mean the widget moves the window
        std::experimental::optional<MirResizeEdge> const resize_edge;
    };

    /// Pointer or touchpoint
    struct Device
    {
        Device(geometry::Point location, bool pressed)
            : location{location},
              pressed{pressed}

        {
        }

        geometry::Point location;
        bool pressed;
        std::experimental::optional<std::shared_ptr<Widget>> active_widget;
    };

    /// The input device has entered the surface
    void process_enter(Device& device);
    /// The input device has left the surface
    void process_leave(Device& device);
    /// The input device has clicked down
    /// A touch triggers a process_enter() followed by a process_down()
    void process_down(Device& device);
    /// The input device has released
    /// A touch release triggers a process_up() followed by a process_leave()
    void process_up(Device& device);
    /// The device has moved while up
    void process_move(Device& device);
    /// The device has moved while down
    void process_drag(Device& device);

    auto widget_at(geometry::Point location) -> std::experimental::optional<std::shared_ptr<Widget>>;

    /// Called when an input device enters the widget
    void widget_enter(Widget& widget);
    /// Called when an input device leaves the widget while up or down
    void widget_leave(Widget& widget);
    /// Called when an input devices it clocked down on the widget
    /// A touch down triggers first a widget_enter() then a widget_down()
    void widget_down(Widget& widget);
    /// Called when an input device is released over a widget it had previously been clicked down on
    void widget_up(Widget& widget);
    /// Called when an input device is dragged within a widget it has clicked down on
    /// Clicking and dragging outside the widget or surface triggers a widget_drag() followed by a widget_leave()
    void widget_drag(Widget& widget);

    void set_cursor(MirResizeEdge resize_edge);

    std::experimental::optional<Device> pointer;
    std::map<int32_t, Device> touches;
    std::vector<std::shared_ptr<Widget>> widgets;
};
}
}
}

#endif // MIR_SHELL_DECORATION_INPUT_H_
