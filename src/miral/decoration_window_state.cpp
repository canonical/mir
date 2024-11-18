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

#include "miral/decoration_window_state.h"
#include "mir/geometry/forward.h"
#include "mir/scene/surface.h"

WindowState::WindowState(StaticGeometry const& static_geometry, ms::Surface const* surface) :
    static_geometry{static_geometry},
    window_size_{surface->window_size()},
    focus_state_{surface->focus_state()},
    window_name_{surface->name()}
{
}

auto WindowState::titlebar_width() const -> geometry::Width
{
    return window_size().width;
}

auto WindowState::titlebar_height() const -> geometry::Height
{
    return static_geometry.titlebar_height;
}

auto WindowState::side_border_width() const -> geometry::Width
{
    return static_geometry.side_border_width;
}

auto WindowState::side_border_height() const -> geometry::Height
{
    return window_size().height - as_delta(titlebar_height()) - as_delta(bottom_border_height());
}

auto WindowState::bottom_border_height() const -> geometry::Height
{
    return static_geometry.bottom_border_height;
}

auto WindowState::bottom_border_width() const -> geometry::Width
{
    return titlebar_width();
}

auto WindowState::titlebar_rect() const -> geometry::Rectangle
{
    return {
        geometry::Point{},
        {titlebar_width(), titlebar_height()}};
}

auto WindowState::left_border_rect() const -> geometry::Rectangle
{
    return {
        {geom::X{}, as_y(titlebar_height())},
        {side_border_width(), side_border_height()}};
}

auto WindowState::right_border_rect() const -> geometry::Rectangle
{
    return {
        {as_x(window_size().width - side_border_width()), as_y(titlebar_height())},
        {side_border_width(), side_border_height()}};
}

auto WindowState::bottom_border_rect() const -> geometry::Rectangle
{
    return {
        {geom::X{}, as_y(window_size().height - bottom_border_height())},
        {bottom_border_width(), bottom_border_height()}};
}

auto WindowState::window_size() const -> geometry::Size
{
    return window_size_;
}

auto WindowState::focused_state() const -> MirWindowFocusState
{
    return focus_state_;
}

auto WindowState::window_name() const -> std::string
{
    return window_name_;
}

auto WindowState::geometry() const -> StaticGeometry
{
    return static_geometry;
}

auto WindowState::resize_corner_input_size() const -> geometry::Size
{
    return static_geometry.resize_corner_input_size;
}

