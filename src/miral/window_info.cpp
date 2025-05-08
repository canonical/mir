/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/window_info.h"
#include "window_info_internal.h"

#include <mir/scene/surface.h>

using namespace mir::geometry;

miral::WindowInfo::WindowInfo() :
    self{std::make_unique<Self>()}
{
}

miral::WindowInfo::WindowInfo(
    Window const& window,
    WindowSpecification const& params) :
    self{std::make_unique<Self>(window, params)}
{
}

miral::WindowInfo::~WindowInfo()
{
}

miral::WindowInfo::WindowInfo(WindowInfo const& that) :
    self{std::make_unique<Self>(*that.self)}
{
}

miral::WindowInfo& miral::WindowInfo::operator=(WindowInfo const& that)
{
    *self = *that.self;
    return *this;
}

bool miral::WindowInfo::can_be_active() const
{
    if (focus_mode() == mir_focus_mode_disabled)
    {
        return false;
    }

    switch (type())
    {
    case mir_window_type_normal:       /**< AKA "regular"                       */
    case mir_window_type_utility:      /**< AKA "floating"                      */
    case mir_window_type_dialog:
    case mir_window_type_freestyle:
    case mir_window_type_menu:
        return true;

    case mir_window_type_satellite:    /**< AKA "toolbox"/"toolbar"             */
    case mir_window_type_inputmethod:  /**< AKA "OSK" or handwriting etc.       */
    case mir_window_type_gloss:
    case mir_window_type_tip:          /**< AKA "tooltip"                       */
    case mir_window_type_decoration:
    default:
        // Cannot have input focus
        return false;
    }
}

bool miral::WindowInfo::must_have_parent() const
{
    switch (type())
    {
    case mir_window_type_gloss:;
    case mir_window_type_inputmethod:
    case mir_window_type_satellite:
    case mir_window_type_tip:
        return true;

    default:
        return false;
    }
}

bool miral::WindowInfo::can_morph_to(MirWindowType new_type) const
{
    switch (new_type)
    {
    case mir_window_type_normal:
    case mir_window_type_utility:
    case mir_window_type_satellite:
        switch (type())
        {
        case mir_window_type_normal:
        case mir_window_type_utility:
        case mir_window_type_dialog:
        case mir_window_type_satellite:
            return true;

        default:
            break;
        }
        break;

    case mir_window_type_dialog:
        switch (type())
        {
        case mir_window_type_normal:
        case mir_window_type_utility:
        case mir_window_type_dialog:
        case mir_window_type_menu:
        case mir_window_type_satellite:
            return true;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return false;
}

bool miral::WindowInfo::must_not_have_parent() const
{
    switch (type())
    {
    case mir_window_type_normal:
    case mir_window_type_utility:
        return true;

    default:
        return false;
    }
}

bool miral::WindowInfo::is_visible() const
{
    switch (state())
    {
    case mir_window_state_hidden:
    case mir_window_state_minimized:
        return false;
    default:
        if (std::shared_ptr<mir::scene::Surface> surface = window())
            return surface->visible();
    }
    return false;
}

void miral::WindowInfo::constrain_resize(Point& requested_pos, Size& requested_size) const
{
    bool const left_resize = requested_pos.x != self->window.top_left().x;
    bool const top_resize  = requested_pos.y != self->window.top_left().y;

    Point new_pos = requested_pos;
    Size new_size = requested_size;

    if (min_width() > new_size.width)
        new_size.width = min_width();

    if (min_height() > new_size.height)
        new_size.height = min_height();

    if (max_width() < new_size.width)
        new_size.width = max_width();

    if (max_height() < new_size.height)
        new_size.height = max_height();

    {
        auto const width = new_size.width.as_int() - min_width().as_int();
        auto inc = width_inc().as_int();
        if (width % inc)
            new_size.width = min_width() + DeltaX{inc*(((2L*width + inc)/2)/inc)};
    }

    {
        auto const height = new_size.height.as_int() - min_height().as_int();
        auto inc = height_inc().as_int();
        if (height % inc)
            new_size.height = min_height() + DeltaY{inc*(((2L*height + inc)/2)/inc)};
    }

    {
        auto const ar = min_aspect();

        auto const error = new_size.height.as_int()*static_cast<long long>(ar.width) -
                           new_size.width.as_int() *static_cast<long long>(ar.height);

        if (error > 0)
        {
            // Add (denominator-1) to numerator to ensure rounding up
            auto const width_correction  = (error+(ar.height-1))/ar.height;
            auto const height_correction = (error+(ar.width-1))/ar.width;

            if (width_correction < height_correction)
            {
                new_size.width = new_size.width + DeltaX(width_correction);
            }
            else
            {
                new_size.height = new_size.height - DeltaY(height_correction);
            }
        }
    }

    {
        auto const ar = max_aspect();

        auto const error = new_size.width.as_int() *static_cast<long long>(ar.height) -
                           new_size.height.as_int()*static_cast<long long>(ar.width);

        if (error > 0)
        {
            // Add (denominator-1) to numerator to ensure rounding up
            auto const height_correction = (error+(ar.width-1))/ar.width;
            auto const width_correction  = (error+(ar.height-1))/ar.height;

            if (width_correction < height_correction)
            {
                new_size.width = new_size.width - DeltaX(width_correction);
            }
            else
            {
                new_size.height = new_size.height + DeltaY(height_correction);
            }
        }
    }

    if (left_resize)
        new_pos.x -= new_size.width - requested_size.width;

    if (top_resize)
        new_pos.y -= new_size.height - requested_size.height;

    // placeholder - constrain onscreen

    requested_pos = new_pos;
    requested_size = new_size;
}

bool miral::WindowInfo::needs_titlebar(MirWindowType type)
{
    switch (type)
    {
    case mir_window_type_freestyle:
    case mir_window_type_menu:
    case mir_window_type_inputmethod:
    case mir_window_type_gloss:
    case mir_window_type_tip:
    case mir_window_type_decoration:
        // No decorations for these surface types
        return false;
    default:
        return true;
    }
}

auto miral::WindowInfo::type() const -> MirWindowType
{
    return self->type;
}

auto miral::WindowInfo::state() const -> MirWindowState
{
    return self->state;
}

auto miral::WindowInfo::restore_rect() const -> mir::geometry::Rectangle
{
    // Calculate a reasonable restore_rect if it hasn't already been explicitly set
    if (!self->restore_rect)
    {
        Size restore_size{};
        // If the window is restored or partially restored, us its current size
        switch (self->state)
        {
        case mir_window_state_restored:
            restore_size = self->window.size();
            break;

        case mir_window_state_vertmaximized:
            restore_size.width = self->window.size().width;
            break;

        case mir_window_state_horizmaximized:
            restore_size.height = self->window.size().height;
            break;

        default:;
        }

        // Halfway between current size and min size is reasonable for a maximized/fullscreen window
        auto const min_size_disp = Displacement{as_delta(self->min_width), as_delta(self->min_height)};
        Size const reasonable_size{as_size(
            (as_displacement(self->window.size()) - min_size_disp) * 0.5
            + min_size_disp)};

        // Set the dimensions that are 0 (unset)
        if (restore_size.width == Width{})
        {
            restore_size.width = reasonable_size.width;
        }

        if (restore_size.height == Height{})
        {
            restore_size.height = reasonable_size.height;
        }

        // Adjust top_left such that the window is shrunk towards the center instead of the top left corner
        Point const restore_top_left{
            self->window.top_left() +
            (as_displacement(self->window.size()) - as_displacement(restore_size)) * 0.5};

        self->restore_rect = Rectangle{restore_top_left, restore_size};
    }
    return self->restore_rect.value();
}

auto miral::WindowInfo::parent() const -> Window
{
    return self->parent;
}

auto miral::WindowInfo::children() const -> std::vector <Window> const&
{
    return self->children;
}

auto miral::WindowInfo::min_width() const -> mir::geometry::Width
{
    return self->min_width;
}

auto miral::WindowInfo::min_height() const -> mir::geometry::Height
{
    return self->min_height;
}

auto miral::WindowInfo::max_width() const -> mir::geometry::Width
{
    return self->max_width;
}

auto miral::WindowInfo::max_height() const -> mir::geometry::Height
{
    return self->max_height;
}

auto miral::WindowInfo::userdata() const -> std::shared_ptr<void>
{
    return self->userdata;
}

void miral::WindowInfo::userdata(std::shared_ptr<void> userdata)
{
    self->userdata = userdata;
}

auto miral::WindowInfo::width_inc() const -> mir::geometry::DeltaX
{
    return self->width_inc;
}

auto miral::WindowInfo::height_inc() const -> mir::geometry::DeltaY
{
    return self->height_inc;
}

auto miral::WindowInfo::min_aspect() const -> AspectRatio
{
    return self->min_aspect;
}

auto miral::WindowInfo::max_aspect() const -> AspectRatio
{
    return self->max_aspect;
}

bool miral::WindowInfo::has_output_id() const
{
    return self->output_id.is_set();
}

auto miral::WindowInfo::output_id() const -> int
{
    return self->output_id.value();
}

auto miral::WindowInfo::window() const -> Window&
{
    return self->window;
}

auto miral::WindowInfo::preferred_orientation() const -> MirOrientationMode
{
    return self->preferred_orientation;
}

auto miral::WindowInfo::confine_pointer() const -> MirPointerConfinementState
{
    return self->confine_pointer;
}

auto miral::WindowInfo::shell_chrome() const -> MirShellChrome
{
    return self->shell_chrome;
}

auto miral::WindowInfo::name() const -> std::string
{
    return self->name;
}

auto miral::WindowInfo::depth_layer() const -> MirDepthLayer
{
    return self->depth_layer;
}

auto miral::WindowInfo::attached_edges() const -> MirPlacementGravity
{
    return self->attached_edges;
}

auto miral::WindowInfo::exclusive_rect() const -> mir::optional_value<mir::geometry::Rectangle>
{
    return self->exclusive_rect;
}

auto miral::WindowInfo::ignore_exclusion_zones() const -> bool
{
    return self->ignore_exclusion_zones;
}

auto miral::WindowInfo::clip_area() const -> mir::optional_value<mir::geometry::Rectangle>
{
    std::shared_ptr<mir::scene::Surface> const surface = self->window;

    if (surface->clip_area())
        return mir::optional_value<mir::geometry::Rectangle>(surface->clip_area().value());
    else
        return mir::optional_value<mir::geometry::Rectangle>();
}

void miral::WindowInfo::clip_area(mir::optional_value<mir::geometry::Rectangle> const& area)
{
    std::shared_ptr<mir::scene::Surface> const surface = self->window;

    if (area)
        surface->set_clip_area(std::optional<mir::geometry::Rectangle>(area.value()));
    else
        surface->set_clip_area(std::optional<mir::geometry::Rectangle>());
}

auto miral::WindowInfo::application_id() const -> std::string
{
    std::shared_ptr<mir::scene::Surface> surface = window();
    if (surface)
        return surface->application_id();
    else
        return "";
}

auto miral::WindowInfo::focus_mode() const -> MirFocusMode
{
    if (std::shared_ptr<mir::scene::Surface> const surface = self->window)
    {
        return surface->focus_mode();
    }
    else
    {
        return mir_focus_mode_disabled;
    }
}

auto miral::WindowInfo::visible_on_lock_screen() const -> bool
{
    std::shared_ptr<mir::scene::Surface> const surface = self->window;
    return surface ? surface->visible_on_lock_screen() : false;
}

auto miral::WindowInfo::tiled_edges() const -> mir::Flags<MirTiledEdge>
{
    std::shared_ptr<mir::scene::Surface> const surface = self->window;
    return surface ? surface->tiled_edges() : mir::Flags(mir_tiled_edge_none);
}
