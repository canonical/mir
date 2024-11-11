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

#ifndef MIRAL_CUSTOM_DECORATIONS_WINDOW_H
#define MIRAL_CUSTOM_DECORATIONS_WINDOW_H

#include "mircore/mir/geometry/forward.h"
#include "mircore/mir/geometry/dimensions.h"
#include "mircore/mir/geometry/size.h"
#include "mircore/mir/geometry/point.h"
#include "mircore/mir/geometry/displacement.h"
#include "mircore/mir_toolkit/common.h"
#include "threadsafe_access.h"

#include <memory>
#include <string>

class UserDecorationExample;

namespace mir
{
namespace scene
{
class Surface;
}
}

enum class BorderType
{
    Full,       ///< Full titlebar and border (for restored windows)
    Titlebar,   ///< Titlebar only (for maximized windows)
    None,       ///< No decorations (for fullscreen windows or popups)
};

/// Decoration geometry properties that don't change
struct StaticGeometry
{
    mir::geometry::Height const titlebar_height;           ///< Visible height of the top titlebar with the window's name and buttons
    mir::geometry::Width const side_border_width;          ///< Visible width of the side borders
    mir::geometry::Height const bottom_border_height;      ///< Visible height of the bottom border
    mir::geometry::Size const resize_corner_input_size;    ///< The size of the input area of a resizable corner
                                                      ///< (does not effect surface input area, only where in the surface is
                                                      ///< considered a resize corner)
    mir::geometry::Width const button_width;               ///< The width of window control buttons
    mir::geometry::Width const padding_between_buttons;    ///< The gep between titlebar buttons
    mir::geometry::Height const title_font_height;         ///< Height of the text used to render the window title
    mir::geometry::Point const title_font_top_left;        ///< Where to render the window title
    mir::geometry::Displacement const icon_padding;        ///< Padding inside buttons around icons
    mir::geometry::Width const icon_line_width;            ///< Width for lines in button icons
};

namespace
{
extern StaticGeometry const default_geometry;
}

/// Information about the geometry and type of decorations for a given window
/// Data is pulled from the surface on construction and immutable after that
class WindowState
{
public:
    WindowState(
        std::shared_ptr<StaticGeometry const> const& static_geometry,
        std::shared_ptr<mir::scene::Surface> const& window_surface,
        float scale);

    auto window_size() const -> mir::geometry::Size;
    auto border_type() const -> BorderType;
    auto focused_state() const -> MirWindowFocusState;
    auto window_name() const -> std::string;

    auto titlebar_width() const -> mir::geometry::Width;
    auto titlebar_height() const -> mir::geometry::Height;

    auto side_border_width() const -> mir::geometry::Width;
    auto side_border_height() const -> mir::geometry::Height;

    auto bottom_border_width() const -> mir::geometry::Width;
    auto bottom_border_height() const -> mir::geometry::Height;

    auto titlebar_rect() const -> mir::geometry::Rectangle;
    auto left_border_rect() const -> mir::geometry::Rectangle;
    auto right_border_rect() const -> mir::geometry::Rectangle;
    auto bottom_border_rect() const -> mir::geometry::Rectangle;

    /// Returns the rectangle of the nth button
    auto button_rect(unsigned n) const -> mir::geometry::Rectangle;

    auto scale() const -> float;

private:
    WindowState(WindowState const&) = delete;
    WindowState& operator=(WindowState const&) = delete;

    std::shared_ptr<StaticGeometry const> const static_geometry;
    mir::geometry::Size const window_size_;
    BorderType const border_type_;
    MirWindowFocusState const focus_state_;
    std::string window_name_;
    float scale_;
};

/// Observes the decorated window and calls on_update when its state changes
class WindowSurfaceObserverManager
{
public:
    WindowSurfaceObserverManager(
        std::shared_ptr<mir::scene::Surface> const& window_surface,
        std::shared_ptr<ThreadsafeAccess<UserDecorationExample>> const& decoration);
    ~WindowSurfaceObserverManager();

private:
    WindowSurfaceObserverManager(WindowSurfaceObserverManager const&) = delete;
    WindowSurfaceObserverManager& operator=(WindowSurfaceObserverManager const&) = delete;

    class Observer;

    std::shared_ptr<mir::scene::Surface> const surface_;
    std::shared_ptr<Observer> const observer;
};

#endif // MIR_SHELL_DECORATION_WINDOW_H_
