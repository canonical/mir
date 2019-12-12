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

#ifndef MIR_SHELL_DECORATION_WINDOW_H_
#define MIR_SHELL_DECORATION_WINDOW_H_

#include "mir/geometry/rectangle.h"

#include <mir_toolkit/common.h>

#include <memory>
#include <functional>

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{
namespace decoration
{
class BasicDecoration;
template<typename T> class ThreadsafeAccess;

enum class BorderType
{
    Full,       ///< Full titlebar and border (for restored windows)
    Titlebar,   ///< Titlebar only (for maximized windows)
    None,       ///< No decorations (for fullscreen windows or popups)
};

/// Decoration geometry properties that don't change
struct StaticGeometry
{
    geometry::Height const titlebar_height;           ///< Visible height of the top titlebar with the window's name and buttons
    geometry::Width const side_border_width;          ///< Visible width of the side borders
    geometry::Height const bottom_border_height;      ///< Visible height of the bottom border
    geometry::Size const resize_corner_input_size;    ///< The size of the input area of a resizable corner
                                                      ///< (does not effect surface input area, only where in the surface is
                                                      ///< considered a resize corner)
    geometry::Width const button_width;               ///< The width of window control buttons
    geometry::Width const padding_between_buttons;    ///< The gep between titlebar buttons
    geometry::Height const title_font_height;         ///< Height of the text used to render the window title
    geometry::Point const title_font_top_left;        ///< Where to render the window title
};

/// Information about the geometry and type of decorations for a given window
/// Data is pulled from the surface on construction and immutable after that
class WindowState
{
public:
    WindowState(
        std::shared_ptr<StaticGeometry const> const& static_geometry,
        std::shared_ptr<scene::Surface> const& window_surface);

    auto window_size() const -> geometry::Size;
    auto border_type() const -> BorderType;
    auto focused_state() const -> MirWindowFocusState;
    auto window_name() const -> std::string;

    auto titlebar_width() const -> geometry::Width;
    auto titlebar_height() const -> geometry::Height;

    auto side_border_width() const -> geometry::Width;
    auto side_border_height() const -> geometry::Height;

    auto bottom_border_width() const -> geometry::Width;
    auto bottom_border_height() const -> geometry::Height;

    auto titlebar_rect() const -> geometry::Rectangle;
    auto left_border_rect() const -> geometry::Rectangle;
    auto right_border_rect() const -> geometry::Rectangle;
    auto bottom_border_rect() const -> geometry::Rectangle;

    /// Returns the rectangle of the nth button
    auto button_rect(unsigned n) const -> geometry::Rectangle;

private:
    WindowState(WindowState const&) = delete;
    WindowState& operator=(WindowState const&) = delete;

    std::shared_ptr<StaticGeometry const> const static_geometry;
    geometry::Size const window_size_;
    BorderType const border_type_;
    MirWindowFocusState const focus_state_;
    std::string window_name_;
};

/// Observes the decorated window and calls on_update when its state changes
class WindowSurfaceObserverManager
{
public:
    WindowSurfaceObserverManager(
        std::shared_ptr<scene::Surface> const& window_surface,
        std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const& decoration);
    ~WindowSurfaceObserverManager();

private:
    WindowSurfaceObserverManager(WindowSurfaceObserverManager const&) = delete;
    WindowSurfaceObserverManager& operator=(WindowSurfaceObserverManager const&) = delete;

    class Observer;

    std::shared_ptr<scene::Surface> const surface_;
    std::shared_ptr<Observer> const observer;
};
}
}
}

#endif // MIR_SHELL_DECORATION_WINDOW_H_
