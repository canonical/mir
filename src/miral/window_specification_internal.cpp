/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "window_specification_internal.h"

namespace
{
template<typename Dest, typename Source>
void copy_if_set(Dest& dest, mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = source.value();
}

template<typename Dest, typename Source>
void copy_if_set(Dest& dest, std::weak_ptr<Source> const& source)
{
    if (source.lock()) dest = source;
}

template<typename Dest, typename Source>
void copy_if_set(mir::optional_value<Dest>& dest, mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = source;
}

template<typename Source>
void copy_if_set(mir::optional_value<mir::graphics::BufferUsage>& dest, mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = static_cast<mir::graphics::BufferUsage>(source.value());
}

template<typename Source>
void copy_if_set(
    mir::optional_value<mir::graphics::DisplayConfigurationOutputId>& dest,
    mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = static_cast<mir::graphics::DisplayConfigurationOutputId>(source.value());
}

template<typename Source>
void copy_if_set(
    mir::optional_value<mir::input::InputReceptionMode>& dest,
    mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = static_cast<mir::input::InputReceptionMode>(source.value());
}

template<typename Source>
void copy_if_set(
    mir::optional_value<mir::shell::SurfaceAspectRatio>& dest,
    mir::optional_value<Source> const& source)
{
    if (source.is_set()) dest = mir::shell::SurfaceAspectRatio{source.value().width, source.value().height};
}

}

auto miral::make_surface_spec(WindowSpecification const& window_spec) -> mir::shell::SurfaceSpecification
{
    auto& spec = *window_spec.self;
    mir::shell::SurfaceSpecification result;
    copy_if_set(result.top_left, spec.top_left);
    copy_if_set(result.pixel_format, spec.pixel_format);
    copy_if_set(result.buffer_usage, spec.buffer_usage);
    copy_if_set(result.name, spec.name);
    copy_if_set(result.output_id, spec.output_id);
    copy_if_set(result.type, spec.type);
    copy_if_set(result.state, spec.state);
    copy_if_set(result.preferred_orientation, spec.preferred_orientation);
    copy_if_set(result.aux_rect, spec.aux_rect);
    copy_if_set(result.min_width, spec.min_width);
    copy_if_set(result.min_height, spec.min_height);
    copy_if_set(result.max_width, spec.max_width);
    copy_if_set(result.max_height, spec.max_height);
    copy_if_set(result.width_inc, spec.width_inc);
    copy_if_set(result.height_inc, spec.height_inc);
    copy_if_set(result.min_aspect, spec.min_aspect);
    copy_if_set(result.max_aspect, spec.max_aspect);
    copy_if_set(result.streams, spec.streams);
    copy_if_set(result.parent, spec.parent);
    copy_if_set(result.input_shape, spec.input_shape);
    copy_if_set(result.input_mode, spec.input_mode);
    copy_if_set(result.shell_chrome, spec.shell_chrome);
    copy_if_set(result.confine_pointer, spec.confine_pointer);
    copy_if_set(result.placement_hints, spec.placement_hints);
    copy_if_set(result.surface_placement_gravity, spec.window_placement_gravity);
    copy_if_set(result.aux_rect_placement_gravity, spec.aux_rect_placement_gravity);
    copy_if_set(result.depth_layer, spec.depth_layer);
    copy_if_set(result.attached_edges, spec.attached_edges);
    copy_if_set(result.exclusive_rect, spec.exclusive_rect);
    copy_if_set(result.application_id, spec.application_id);
    copy_if_set(result.server_side_decorated, spec.server_side_decorated);
    copy_if_set(result.focus_mode, spec.focus_mode);
    copy_if_set(result.visible_on_lock_screen, spec.visible_on_lock_screen);

    if (spec.size.is_set())
    {
        result.set_size(spec.size.value());
    }

    if (spec.aux_rect_placement_offset.is_set())
    {
        auto const offset = spec.aux_rect_placement_offset.value();
        result.aux_rect_placement_offset_x = offset.dx.as_int();
        result.aux_rect_placement_offset_y = offset.dy.as_int();
    }

    return result;
}
