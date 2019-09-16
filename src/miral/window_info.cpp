/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/window_info.h"
#include "window_info_defaults.h"

#include <mir/scene/surface.h>

#include <limits>

using namespace mir::geometry;

namespace
{
template<typename Value>
auto optional_value_or_default(mir::optional_value<Value> const& optional_value, Value default_ = Value{})
-> Value
{
    return optional_value.is_set() ? optional_value.value() : default_;
}

unsigned clamp_dim(unsigned dim)
{
    return std::min<unsigned long>(std::numeric_limits<long>::max(), dim);
}

// For our convenience when doing calculations we clamp the dimensions of the aspect ratio
// so they will fit into a long without overflow.
miral::WindowInfo::AspectRatio clamp(miral::WindowInfo::AspectRatio const& source)
{
    return {clamp_dim(source.width), clamp_dim(source.height)};
}

miral::Width clamp(miral::Width const& source)
{
    return std::min(miral::default_max_width, std::max(miral::default_min_width, source));
}

miral::Height clamp(miral::Height const& source)
{
    return std::min(miral::default_max_height, std::max(miral::default_min_height, source));
}
}

miral::Width  const miral::default_min_width{0};
miral::Height const miral::default_min_height{0};
miral::Width  const miral::default_max_width{std::numeric_limits<int>::max()};
miral::Height const miral::default_max_height{std::numeric_limits<int>::max()};
miral::DeltaX const miral::default_width_inc{1};
miral::DeltaY const miral::default_height_inc{1};
miral::WindowInfo::AspectRatio const miral::default_min_aspect_ratio{
    clamp(WindowInfo::AspectRatio{0U, std::numeric_limits<unsigned>::max()})};
miral::WindowInfo::AspectRatio const miral::default_max_aspect_ratio{
    clamp(WindowInfo::AspectRatio{std::numeric_limits<unsigned>::max(), 0U})};

struct miral::WindowInfo::Self
{
    Self(Window window, WindowSpecification const& params);
    Self();

    Window window;
    std::string name;
    MirWindowType type;
    MirWindowState state;
    mir::geometry::Rectangle restore_rect;
    Window parent;
    std::vector <Window> children;
    mir::geometry::Width min_width;
    mir::geometry::Height min_height;
    mir::geometry::Width max_width;
    mir::geometry::Height max_height;
    MirOrientationMode preferred_orientation;
    MirPointerConfinementState confine_pointer;

    mir::geometry::DeltaX width_inc;
    mir::geometry::DeltaY height_inc;
    AspectRatio min_aspect;
    AspectRatio max_aspect;
    mir::optional_value<int> output_id;
    MirShellChrome shell_chrome;
    MirDepthLayer depth_layer;
    MirPlacementGravity attached_edges;
    mir::optional_value<mir::geometry::Rectangle> exclusive_rect;
    std::shared_ptr<void> userdata;
};

miral::WindowInfo::Self::Self(Window window, WindowSpecification const& params) :
    window{window},
    name{params.name().value()},
    type{optional_value_or_default(params.type(), mir_window_type_normal)},
    state{optional_value_or_default(params.state(), mir_window_state_restored)},
    restore_rect{params.top_left().value(), params.size().value()},
    min_width{optional_value_or_default(params.min_width(), miral::default_min_width)},
    min_height{optional_value_or_default(params.min_height(), miral::default_min_height)},
    max_width{optional_value_or_default(params.max_width(), miral::default_max_width)},
    max_height{optional_value_or_default(params.max_height(), miral::default_max_height)},
    preferred_orientation{optional_value_or_default(params.preferred_orientation(), mir_orientation_mode_any)},
    confine_pointer(optional_value_or_default(params.confine_pointer(), mir_pointer_unconfined)),
    width_inc{optional_value_or_default(params.width_inc(), default_width_inc)},
    height_inc{optional_value_or_default(params.height_inc(), default_height_inc)},
    min_aspect(optional_value_or_default(params.min_aspect(), default_min_aspect_ratio)),
    max_aspect(optional_value_or_default(params.max_aspect(), default_max_aspect_ratio)),
    shell_chrome(optional_value_or_default(params.shell_chrome(), mir_shell_chrome_normal)),
    depth_layer(optional_value_or_default(params.depth_layer(), mir_depth_layer_application)),
    attached_edges(optional_value_or_default(params.attached_edges(), mir_placement_gravity_center))
{
    if (params.output_id().is_set())
        output_id = params.output_id().value();

    if (params.exclusive_rect().is_set())
        exclusive_rect = params.exclusive_rect().value();

    if (params.userdata().is_set())
        userdata = params.userdata().value();
}

miral::WindowInfo::Self::Self() :
    type{mir_window_type_normal},
    state{mir_window_state_unknown},
    preferred_orientation{mir_orientation_mode_any},
    shell_chrome{mir_shell_chrome_normal}
{
}

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
        new_pos.x += new_size.width - requested_size.width;

    if (top_resize)
        new_pos.y += new_size.height - requested_size.height;

    // placeholder - constrain onscreen

    switch (state())
    {
    case mir_window_state_restored:
        break;

        // "A vertically maximised window is anchored to the top and bottom of
        // the available workspace and can have any width."
    case mir_window_state_vertmaximized:
        new_pos.y = self->window.top_left().y;
        new_size.height = self->window.size().height;
        break;

        // "A horizontally maximised window is anchored to the left and right of
        // the available workspace and can have any height"
    case mir_window_state_horizmaximized:
        new_pos.x = self->window.top_left().x;
        new_size.width = self->window.size().width;
        break;

        // "A maximised window is anchored to the top, bottom, left and right of the
        // available workspace. For example, if the launcher is always-visible then
        // the left-edge of the window is anchored to the right-edge of the launcher."
    case mir_window_state_maximized:
    default:
        new_pos.x = self->window.top_left().x;
        new_pos.y = self->window.top_left().y;
        new_size.width = self->window.size().width;
        new_size.height = self->window.size().height;
        break;
    }

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

void miral::WindowInfo::type(MirWindowType type)
{
    self->type = type;
}

auto miral::WindowInfo::state() const -> MirWindowState
{
    return self->state;
}

void miral::WindowInfo::state(MirWindowState state)
{
    self->state = state;
}

auto miral::WindowInfo::restore_rect() const -> mir::geometry::Rectangle
{
    return self->restore_rect;
}

void miral::WindowInfo::restore_rect(mir::geometry::Rectangle const& restore_rect)
{
    self->restore_rect = restore_rect;
}

auto miral::WindowInfo::parent() const -> Window
{
    return self->parent;
}

void miral::WindowInfo::parent(Window const& parent)
{
    self->parent = parent;
}

auto miral::WindowInfo::children() const -> std::vector <Window> const&
{
    return self->children;
}

void miral::WindowInfo::add_child(Window const& child)
{
    self->children.push_back(child);
}

void miral::WindowInfo::remove_child(Window const& child)
{
    auto& siblings = self->children;

    for (auto i = begin(siblings); i != end(siblings); ++i)
    {
        if (child == *i)
        {
            siblings.erase(i);
            break;
        }
    }
}

auto miral::WindowInfo::min_width() const -> mir::geometry::Width
{
    return self->min_width;
}

void miral::WindowInfo::min_width(mir::geometry::Width min_width)
{
    self->min_width = clamp(min_width);
}

auto miral::WindowInfo::min_height() const -> mir::geometry::Height
{
    return self->min_height;
}

void miral::WindowInfo::min_height(mir::geometry::Height min_height)
{
    self->min_height = clamp(min_height);
}

auto miral::WindowInfo::max_width() const -> mir::geometry::Width
{
    return self->max_width;
}

void miral::WindowInfo::max_width(mir::geometry::Width max_width)
{
    self->max_width = clamp(max_width);
}

auto miral::WindowInfo::max_height() const -> mir::geometry::Height
{
    return self->max_height;
}

void miral::WindowInfo::max_height(mir::geometry::Height max_height)
{
    self->max_height = clamp(max_height);
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

void miral::WindowInfo::width_inc(DeltaX width_inc)
{
    self->width_inc = width_inc;
}

auto miral::WindowInfo::height_inc() const -> mir::geometry::DeltaY
{
    return self->height_inc;
}

void miral::WindowInfo::height_inc(mir::geometry::DeltaY height_inc)
{
    self->height_inc = height_inc;
}

auto miral::WindowInfo::min_aspect() const -> AspectRatio
{
    return self->min_aspect;
}

void miral::WindowInfo::min_aspect(AspectRatio min_aspect)
{
    self->min_aspect = clamp(min_aspect);
}

auto miral::WindowInfo::max_aspect() const -> AspectRatio
{
    return self->max_aspect;
}

void miral::WindowInfo::max_aspect(AspectRatio max_aspect)
{
    self->max_aspect = clamp(max_aspect);
}

bool miral::WindowInfo::has_output_id() const
{
    return self->output_id.is_set();
}

auto miral::WindowInfo::output_id() const -> int
{
    return self->output_id.value();
}

void miral::WindowInfo::output_id(mir::optional_value<int> output_id)
{
    self->output_id = output_id;
}

auto miral::WindowInfo::window() const -> Window&
{
    return self->window;
}

auto miral::WindowInfo::preferred_orientation() const -> MirOrientationMode
{
    return self->preferred_orientation;
}

void miral::WindowInfo::preferred_orientation(MirOrientationMode preferred_orientation)
{
    self->preferred_orientation = preferred_orientation;
}

auto miral::WindowInfo::confine_pointer() const -> MirPointerConfinementState
{
    return self->confine_pointer;
}

void miral::WindowInfo::confine_pointer(MirPointerConfinementState confinement)
{
    self->confine_pointer = confinement;
}

auto miral::WindowInfo::shell_chrome() const -> MirShellChrome
{
    return self->shell_chrome;
}

void miral::WindowInfo::shell_chrome(MirShellChrome chrome)
{
    self->shell_chrome = chrome;
}

auto miral::WindowInfo::name() const -> std::string
{
    return self->name;
}

void miral::WindowInfo::name(std::string const& name)
{
    self->name = name;
}

auto miral::WindowInfo::depth_layer() const -> MirDepthLayer
{
    return self->depth_layer;
}

void miral::WindowInfo::depth_layer(MirDepthLayer depth_layer)
{
    self->depth_layer = depth_layer;
}

auto miral::WindowInfo::attached_edges() const -> MirPlacementGravity
{
    return self->attached_edges;
}

void miral::WindowInfo::attached_edges(MirPlacementGravity edges)
{
    self->attached_edges = edges;
}

auto miral::WindowInfo::exclusive_rect() const -> mir::optional_value<mir::geometry::Rectangle>
{
    return self->exclusive_rect;
}

void miral::WindowInfo::exclusive_rect(mir::optional_value<mir::geometry::Rectangle> const& rect)
{
    self->exclusive_rect = rect;
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
        surface->set_clip_area(std::experimental::optional<mir::geometry::Rectangle>(area.value()));
    else
        surface->set_clip_area(std::experimental::optional<mir::geometry::Rectangle>());
}
