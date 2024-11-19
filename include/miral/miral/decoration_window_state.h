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

#ifndef MIRAL_DECORATION_WINDOW_STATE_H
#define MIRAL_DECORATION_WINDOW_STATE_H

#include "mir/geometry/dimensions.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/forward.h"
#include "mir/geometry/point.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

namespace mir::scene { class Surface; }

namespace ms = mir::scene;
namespace geometry = mir::geometry;

enum class BorderType
{
    Full,       ///< Full titlebar and border (for restored windows)
    Titlebar,   ///< Titlebar only (for maximized windows)
    None,       ///< No decorations (for fullscreen windows or popups)
};

/// Decoration geometry properties that don't change
struct StaticGeometry
{
    geometry::Height const
        titlebar_height; ///< Visible height of the top titlebar with the window's name and buttons
    geometry::Width const side_border_width;       ///< Visible width of the side borders
    geometry::Height const bottom_border_height;   ///< Visible height of the bottom border
    geometry::Size const resize_corner_input_size; ///< The size of the input area of a resizable corner
                                                   ///< (does not effect surface input area, only where in the
                                                   ///< surface is considered a resize corner)
    geometry::Width const button_width;            ///< The width of window control buttons
    geometry::Width const padding_between_buttons; ///< The gep between titlebar buttons
    geometry::Height const title_font_height;      ///< Height of the text used to render the window title
    geometry::Point const title_font_top_left;     ///< Where to render the window title
    geometry::Displacement const icon_padding;     ///< Padding inside buttons around icons
    geometry::Width const icon_line_width;         ///< Width for lines in button icons
};

class WindowState
{
public:
    WindowState(StaticGeometry const& static_geometry, ms::Surface const* surface);

    auto window_size() const -> geometry::Size;
    auto focused_state() const -> MirWindowFocusState;
    auto window_name() const -> std::string;

    auto titlebar_width() const -> geometry::Width;
    auto titlebar_height() const -> geometry::Height;

    auto side_border_width() const -> geometry::Width;
    auto side_border_height() const -> geometry::Height;

    auto bottom_border_height() const -> geometry::Height;
    auto bottom_border_width() const -> geometry::Width;

    auto titlebar_rect() const -> geometry::Rectangle;
    auto left_border_rect() const -> geometry::Rectangle;
    auto right_border_rect() const -> geometry::Rectangle;
    auto bottom_border_rect() const -> geometry::Rectangle;

    auto geometry() const -> StaticGeometry;
    auto resize_corner_input_size() const -> geometry::Size;
    auto border_type() const -> BorderType;

private:
    WindowState(WindowState const&) = delete;
    WindowState& operator=(WindowState const&) = delete;

    StaticGeometry const static_geometry; // TODO: this can be shared between all instance of a decoration
    geometry::Size const window_size_;
    BorderType const border_type_;
    MirWindowFocusState const focus_state_;
    std::string window_name_;
};

#endif
