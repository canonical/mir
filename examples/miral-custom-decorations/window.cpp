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

#include "window.h"

#include "mir/scene/surface.h"
#include "mir/shell/decoration.h"

namespace geom = mir::geometry;

namespace
{
auto border_type_for(MirWindowType type, MirWindowState state) -> BorderType
{
    using BorderType = BorderType;

    switch (type)
    {
    case mir_window_type_normal:
    case mir_window_type_utility:
    case mir_window_type_dialog:
    case mir_window_type_freestyle:
    case mir_window_type_satellite:
        break;

    case mir_window_type_gloss:
    case mir_window_type_menu:
    case mir_window_type_inputmethod:
    case mir_window_type_tip:
    case mir_window_type_decoration:
    case mir_window_types:
        return BorderType::None;
    }

    switch (state)
    {
    case mir_window_state_unknown:
    case mir_window_state_restored:
        return BorderType::Full;

    case mir_window_state_maximized:
    case mir_window_state_vertmaximized:
    case mir_window_state_horizmaximized:
        return BorderType::Titlebar;

    case mir_window_state_minimized:
    case mir_window_state_fullscreen:
    case mir_window_state_hidden:
    case mir_window_state_attached:
    case mir_window_states:
        return BorderType::None;
    }

    mir::fatal_error("%s:%d: should be unreachable", __FILE__, __LINE__);
    return {};
}
}

WindowState::WindowState(
    std::shared_ptr<StaticGeometry const> const& static_geometry,
    std::shared_ptr<mir::scene::Surface> const& surface,
    float scale)
    : static_geometry{static_geometry},
      window_size_{surface->window_size()},
      border_type_{border_type_for(surface->type(), surface->state())},
      focus_state_{surface->focus_state()},
      window_name_{surface->name()},
      scale_{scale}
{
}

auto WindowState::window_size() const -> geom::Size
{
    return window_size_;
}

auto WindowState::border_type() const -> BorderType
{
    return border_type_;
}

auto WindowState::focused_state() const -> MirWindowFocusState
{
    return focus_state_;
}

auto WindowState::window_name() const -> std::string
{
    return window_name_;
}

auto WindowState::titlebar_width() const -> geom::Width
{
    switch (border_type_)
    {
    case BorderType::Full:
    case BorderType::Titlebar:
        return window_size().width;
    case BorderType::None:
        return {};
    }

    mir::fatal_error("%s:%d: should be unreachable", __FILE__, __LINE__);
    return {};
}

auto WindowState::titlebar_height() const -> geom::Height
{
    switch (border_type_)
    {
    case BorderType::Full:
    case BorderType::Titlebar:
        return static_geometry->titlebar_height;
    case BorderType::None:
        return {};
    }

    mir::fatal_error("%s:%d: should be unreachable", __FILE__, __LINE__);
    return {};
}

auto WindowState::side_border_width() const -> geom::Width
{
    switch (border_type_)
    {
    case BorderType::Full:
        return static_geometry->side_border_width;
    case BorderType::Titlebar:
    case BorderType::None:
        return {};
    }

    mir::fatal_error("%s:%d: should be unreachable", __FILE__, __LINE__);
    return {};
}

auto WindowState::side_border_height() const -> geom::Height
{
    switch (border_type_)
    {
    case BorderType::Full:
        return window_size().height - as_delta(titlebar_height()) - as_delta(bottom_border_height());
    case BorderType::Titlebar:
    case BorderType::None:
        return {};
    }

    mir::fatal_error("%s:%d: should be unreachable", __FILE__, __LINE__);
    return {};
}

auto WindowState::bottom_border_width() const -> geom::Width
{
    return titlebar_width();
}

auto WindowState::bottom_border_height() const -> geom::Height
{
    switch (border_type_)
    {
    case BorderType::Full:
        return static_geometry->bottom_border_height;
    case BorderType::Titlebar:
    case BorderType::None:
        return {};
    }

    mir::fatal_error("%s:%d: should be unreachable", __FILE__, __LINE__);
    return {};
}

auto WindowState::titlebar_rect() const -> geom::Rectangle
{
    return {
        geom::Point{},
        {titlebar_width(), titlebar_height()}};
}

auto WindowState::left_border_rect() const -> geom::Rectangle
{
    return {
        {geom::X{}, as_y(titlebar_height())},
        {side_border_width(), side_border_height()}};
}

auto WindowState::right_border_rect() const -> geom::Rectangle
{
    return {
        {as_x(window_size().width - side_border_width()), as_y(titlebar_height())},
        {side_border_width(), side_border_height()}};
}

auto WindowState::bottom_border_rect() const -> geom::Rectangle
{
    return {
        {geom::X{}, as_y(window_size().height - bottom_border_height())},
        {bottom_border_width(), bottom_border_height()}};
}

auto WindowState::button_rect(unsigned n) const -> geom::Rectangle
{
    geom::Rectangle titlebar = titlebar_rect();
    geom::X x =
        titlebar.right() -
        as_delta(side_border_width()) -
        n * as_delta(static_geometry->button_width + static_geometry->padding_between_buttons) -
        as_delta(static_geometry->button_width);
    return geom::Rectangle{
        {x, titlebar.top()},
        {static_geometry->button_width, titlebar.size.height}};
}

auto WindowState::scale() const -> float
{
    return scale_;
}
