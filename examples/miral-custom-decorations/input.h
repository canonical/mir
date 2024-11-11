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

#ifndef MIRAL_CUSTOM_DECORATIONS_INPUT_H
#define MIRAL_CUSTOM_DECORATIONS_INPUT_H

#include "mir/shell/input_resolver.h"
#include "threadsafe_access.h"

#include <memory>
#include <vector>
#include <mutex>
#include <optional>

namespace msd = mir::shell::decoration;

struct MirEvent;
class UserDecorationExample;
struct StaticGeometry;
class WindowState;

template<typename T>
class ThreadsafeAccess;

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
}
}

enum class ButtonFunction
{
    Close,
    Maximize,
    Minimize,
};

struct ButtonInfo
{
    ButtonFunction function;
    msd::ButtonState state;
    mir::geometry::Rectangle rect;

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
        std::vector<mir::geometry::Rectangle> const& input_shape)
        : buttons_{buttons},
          input_shape_{input_shape}
    {
    }

    auto buttons() const -> std::vector<ButtonInfo> const& { return buttons_; }
    auto input_shape() const -> std::vector<mir::geometry::Rectangle> const& { return input_shape_; }

private:
    InputState(InputState const&) = delete;
    InputState& operator=(InputState const&) = delete;

    std::vector<ButtonInfo> const buttons_;
    std::vector<mir::geometry::Rectangle> const input_shape_;
};

/// Manages user input
class InputManager
{

public:
    InputManager(
        std::shared_ptr<StaticGeometry const> const& static_geometry,
        WindowState const& window_state,
        std::shared_ptr<ThreadsafeAccess<UserDecorationExample>> const& decoration);
    ~InputManager();

    void handle_input_event(std::shared_ptr<MirEvent const> const& event);

    void update_window_state(WindowState const& window_state);
    auto state() -> std::unique_ptr<InputState>;

private:
    static std::chrono::nanoseconds const double_click_threshold;

    InputManager(InputManager const&) = delete;
    InputManager& operator=(InputManager const&) = delete;

    static auto resize_edge_rect(
        WindowState const& window_state,
        StaticGeometry const& static_geometry,
        MirResizeEdge resize_edge) -> mir::geometry::Rectangle;

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

        mir::geometry::Rectangle rect;
        msd::ButtonState state{msd::ButtonState::Up};
        std::optional<ButtonFunction> const button;
        // mir_resize_edge_none is used to mean the widget moves the window
        std::optional<MirResizeEdge> const resize_edge;
    };

    using DeviceEvent = msd::InputResolver::DeviceEvent;
    /// Pointer or touchpoint
    /// The input device has entered the surface
    void process_enter(DeviceEvent& device);
    /// The input device has left the surface
    void process_leave();
    /// The input device has clicked down
    /// A touch triggers a process_enter() followed by a process_down()
    void process_down();
    /// The input device has released
    /// A touch release triggers a process_up() followed by a process_leave()
    void process_up();
    /// The device has moved while up
    void process_move(DeviceEvent& device);
    /// The device has moved while down
    void process_drag(DeviceEvent& device);

    auto widget_at(mir::geometry::Point location) -> std::optional<std::shared_ptr<Widget>>;

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

    std::shared_ptr<StaticGeometry const> const static_geometry;
    std::shared_ptr<ThreadsafeAccess<UserDecorationExample>> decoration;
    std::vector<std::shared_ptr<Widget>> widgets;
    std::unique_ptr<mir::shell::decoration::InputResolver> input_resolver;
    std::mutex mutex;
    std::vector<mir::geometry::Rectangle> input_shape;
    std::optional<std::shared_ptr<Widget>> active_widget;
    /// Timestamp of the previous up event (not the one currently being processed)
    std::chrono::nanoseconds previous_up_timestamp{0};
    /// The most recent event, or the event currently being processed
    std::string current_cursor_name{""};
};

#endif
