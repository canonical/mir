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

#include "window_info_internal.h"
#include "window_info_defaults.h"

#include <mir/scene/surface.h>

#include <limits>

using namespace mir::geometry;

namespace
{
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
    clamp(WindowInfo::AspectRatio{1U, std::numeric_limits<unsigned>::max()})};
miral::WindowInfo::AspectRatio const miral::default_max_aspect_ratio{
    clamp(WindowInfo::AspectRatio{std::numeric_limits<unsigned>::max(), 1U})};

miral::WindowInfo::Self::Self(Window window, WindowSpecification const& params) :
    window{window},
    name{params.name().value_or({""})},
    type{params.type().value_or(mir_window_type_normal)},
    state{params.state().value_or(mir_window_state_restored)},
    restore_rect{},
    min_width{params.min_width().value_or(miral::default_min_width)},
    min_height{params.min_height().value_or(miral::default_min_height)},
    max_width{params.max_width().value_or(miral::default_max_width)},
    max_height{params.max_height().value_or(miral::default_max_height)},
    preferred_orientation{params.preferred_orientation().value_or(mir_orientation_mode_any)},
    confine_pointer(params.confine_pointer().value_or(mir_pointer_unconfined)),
    width_inc{params.width_inc().value_or(default_width_inc)},
    height_inc{params.height_inc().value_or(default_height_inc)},
    min_aspect(params.min_aspect().value_or(default_min_aspect_ratio)),
    max_aspect(params.max_aspect().value_or(default_max_aspect_ratio)),
    shell_chrome(params.shell_chrome().value_or(mir_shell_chrome_normal)),
    depth_layer(params.depth_layer().value_or(mir_depth_layer_application)),
    attached_edges(params.attached_edges().value_or(mir_placement_gravity_center)),
    ignore_exclusion_zones{params.ignore_exclusion_zones().value_or(false)}
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

void miral::WindowInfo::name(std::string const& name)
{
    self->name = name;
}

void miral::WindowInfo::type(MirWindowType type)
{
    self->type = type;
}

void miral::WindowInfo::state(MirWindowState state)
{
    self->state = state;
}

void miral::WindowInfo::restore_rect(mir::geometry::Rectangle const& restore_rect)
{
    self->restore_rect = restore_rect;
}

void miral::WindowInfo::parent(Window const& parent)
{
    self->parent = parent;
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

void miral::WindowInfo::min_width(mir::geometry::Width min_width)
{
    self->min_width = clamp(min_width);
}

void miral::WindowInfo::min_height(mir::geometry::Height min_height)
{
    self->min_height = clamp(min_height);
}

void miral::WindowInfo::max_width(mir::geometry::Width max_width)
{
    self->max_width = clamp(max_width);
}

void miral::WindowInfo::max_height(mir::geometry::Height max_height)
{
    self->max_height = clamp(max_height);
}

void miral::WindowInfo::width_inc(DeltaX width_inc)
{
    self->width_inc = width_inc;
}

void miral::WindowInfo::height_inc(mir::geometry::DeltaY height_inc)
{
    self->height_inc = height_inc;
}

void miral::WindowInfo::min_aspect(AspectRatio min_aspect)
{
    self->min_aspect = clamp(min_aspect);
}

void miral::WindowInfo::max_aspect(AspectRatio max_aspect)
{
    self->max_aspect = clamp(max_aspect);
}

void miral::WindowInfo::output_id(mir::optional_value<int> output_id)
{
    self->output_id = output_id;
}

void miral::WindowInfo::preferred_orientation(MirOrientationMode preferred_orientation)
{
    self->preferred_orientation = preferred_orientation;
}

void miral::WindowInfo::confine_pointer(MirPointerConfinementState confinement)
{
    self->confine_pointer = confinement;
}

void miral::WindowInfo::shell_chrome(MirShellChrome chrome)
{
    self->shell_chrome = chrome;
}

void miral::WindowInfo::depth_layer(MirDepthLayer depth_layer)
{
    self->depth_layer = depth_layer;
}

void miral::WindowInfo::attached_edges(MirPlacementGravity edges)
{
    self->attached_edges = edges;
}

void miral::WindowInfo::exclusive_rect(mir::optional_value<mir::geometry::Rectangle> const& rect)
{
    self->exclusive_rect = rect;
}

void miral::WindowInfo::application_id(std::string const& application_id)
{
    std::shared_ptr<mir::scene::Surface> surface = window();
    if (surface)
        return surface->set_application_id(application_id);
}

void miral::WindowInfo::focus_mode(MirFocusMode focus_mode)
{
    if (std::shared_ptr<mir::scene::Surface> const surface = self->window)
        surface->set_focus_mode(focus_mode);
}

void miral::WindowInfo::visible_on_lock_screen(bool visible)
{
    if (std::shared_ptr<mir::scene::Surface> const surface = self->window)
        surface->set_visible_on_lock_screen(visible);
}
