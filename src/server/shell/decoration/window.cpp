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

#include "window.h"
#include "threadsafe_access.h"
#include "basic_decoration.h"

#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"

namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

namespace
{
auto border_type_for(std::shared_ptr<ms::Surface> const& surface) -> msd::BorderType
{
    using BorderType = msd::BorderType;

    switch (surface->type())
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

    switch (surface->state())
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

msd::WindowState::WindowState(
    std::shared_ptr<StaticGeometry const> const& static_geometry,
    std::shared_ptr<scene::Surface> const& surface)
    : static_geometry{static_geometry},
      window_size_{surface->window_size()},
      border_type_{border_type_for(surface)},
      focus_state_{surface->focus_state()},
      window_name_{surface->name()}
{
}

auto msd::WindowState::window_size() const -> geom::Size
{
    return window_size_;
}

auto msd::WindowState::border_type() const -> BorderType
{
    return border_type_;
}

auto msd::WindowState::focused_state() const -> MirWindowFocusState
{
    return focus_state_;
}

auto msd::WindowState::window_name() const -> std::string
{
    return window_name_;
}

auto msd::WindowState::titlebar_width() const -> geom::Width
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

auto msd::WindowState::titlebar_height() const -> geom::Height
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

auto msd::WindowState::side_border_width() const -> geom::Width
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

auto msd::WindowState::side_border_height() const -> geom::Height
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

auto msd::WindowState::bottom_border_width() const -> geom::Width
{
    return titlebar_width();
}

auto msd::WindowState::bottom_border_height() const -> geom::Height
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

auto msd::WindowState::titlebar_rect() const -> geom::Rectangle
{
    return {
        geom::Point{},
        {titlebar_width(), titlebar_height()}};
}

auto msd::WindowState::left_border_rect() const -> geom::Rectangle
{
    return {
        {geom::X{}, as_y(titlebar_height())},
        {side_border_width(), side_border_height()}};
}

auto msd::WindowState::right_border_rect() const -> geom::Rectangle
{
    return {
        {as_x(window_size().width - side_border_width()), as_y(titlebar_height())},
        {side_border_width(), side_border_height()}};
}

auto msd::WindowState::bottom_border_rect() const -> geom::Rectangle
{
    return {
        {geom::X{}, as_y(window_size().height - bottom_border_height())},
        {bottom_border_width(), bottom_border_height()}};
}

auto msd::WindowState::button_rect(unsigned n) const -> geom::Rectangle
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

class msd::WindowSurfaceObserverManager::Observer
    : public ms::NullSurfaceObserver
{
public:
    Observer(std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const& decoration)
        : decoration{decoration}
    {
    }

    /// Overrides from NullSurfaceObserver
    /// @{
    void attrib_changed(ms::Surface const*, MirWindowAttrib attrib, int /*value*/) override
    {
        switch(attrib)
        {
        case mir_window_attrib_type:
        case mir_window_attrib_state:
        case mir_window_attrib_focus:
        case mir_window_attrib_visibility:
            update();
            break;

        case mir_window_attrib_swapinterval:
        case mir_window_attrib_dpi:
        case mir_window_attrib_preferred_orientation:
        case mir_window_attribs:
            break;
        }
    }

    void window_resized_to(ms::Surface const*, geom::Size const& /*window_size*/) override
    {
        update();
    }

    void renamed(ms::Surface const*, char const* /*name*/) override
    {
        update();
    }
    /// @}

private:
    void update()
    {
        decoration->spawn([](auto decoration)
            {
                decoration->window_state_updated();
            });
    }

    std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const decoration;
};

msd::WindowSurfaceObserverManager::WindowSurfaceObserverManager(
    std::shared_ptr<scene::Surface> const& window_surface,
    std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const& decoration)
    : surface_{window_surface},
      observer{std::make_shared<Observer>(decoration)}
{
    surface_->add_observer(observer);
}

msd::WindowSurfaceObserverManager::~WindowSurfaceObserverManager()
{
    surface_->remove_observer(observer);
}
