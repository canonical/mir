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

#include <tuple>

#include <mir/shell/surface_specification.h>

namespace msh = mir::shell;

auto msh::operator==(StreamSpecification const& lhs, StreamSpecification const& rhs) -> bool
{
    return std::make_tuple(lhs.stream.lock(), lhs.displacement) ==
           std::make_tuple(rhs.stream.lock(), rhs.displacement);
}

bool msh::SurfaceSpecification::is_empty() const
{
    // You know, compile-time reflection would be pretty
    // useful here :)
    return !top_left.is_set() &&
        !width.is_set() &&
        !height.is_set() &&
        !pixel_format.is_set() &&
        !buffer_usage.is_set() &&
        !name.is_set() &&
        !output_id.is_set() &&
        !type.is_set() &&
        !state.is_set() &&
        !preferred_orientation.is_set() &&
        !parent_id.is_set() &&
        !aux_rect.is_set() &&
        !edge_attachment.is_set() &&
        !min_width.is_set() &&
        !min_height.is_set() &&
        !max_width.is_set() &&
        !max_height.is_set() &&
        !width_inc.is_set() &&
        !height_inc.is_set() &&
        !min_aspect.is_set() &&
        !max_aspect.is_set() &&
        !streams.is_set() &&
        !parent.is_set() &&
        !input_shape.is_set() &&
        !input_mode.is_set() &&
        !shell_chrome.is_set() &&
        !depth_layer.is_set() &&
        !attached_edges.is_set() &&
        !exclusive_rect.is_set() &&
        !application_id.is_set() &&
        !server_side_decorated.is_set() &&
        !focus_mode.is_set() &&
        !visible_on_lock_screen.is_set() &&
        !tiled_edges.is_set() &&
        !alpha.is_set() &&
        !parent_size.is_set();
}

void msh::SurfaceSpecification::update_from(SurfaceSpecification const& that)
{
    if (that.top_left.is_set())
        top_left = that.top_left;
    if (that.width.is_set())
        width = that.width;
    if (that.height.is_set())
        height = that.height;
    if (that.pixel_format.is_set())
        pixel_format = that.pixel_format;
    if (that.buffer_usage.is_set())
        buffer_usage = that.buffer_usage;
    if (that.name.is_set())
        name = that.name;
    if (that.output_id.is_set())
        output_id = that.output_id;
    if (that.type.is_set())
        type = that.type;
    if (that.state.is_set())
        state = that.state;
    if (that.preferred_orientation.is_set())
        preferred_orientation = that.preferred_orientation;
    if (that.parent_id.is_set())
        parent_id = that.parent_id;
    if (that.aux_rect.is_set())
        aux_rect = that.aux_rect;
    if (that.edge_attachment.is_set())
        edge_attachment = that.edge_attachment;
    if (that.placement_hints.is_set())
        placement_hints = that.placement_hints;
    if (that.surface_placement_gravity.is_set())
        surface_placement_gravity = that.surface_placement_gravity;
    if (that.aux_rect_placement_gravity.is_set())
        aux_rect_placement_gravity = that.aux_rect_placement_gravity;
    if (that.aux_rect_placement_offset_x.is_set())
        aux_rect_placement_offset_x = that.aux_rect_placement_offset_x;
    if (that.aux_rect_placement_offset_y.is_set())
        aux_rect_placement_offset_y = that.aux_rect_placement_offset_y;
    if (that.min_width.is_set())
        min_width = that.min_width;
    if (that.min_height.is_set())
        min_height = that.min_height;
    if (that.max_width.is_set())
        max_width = that.max_width;
    if (that.max_height.is_set())
        max_height = that.max_height;
    if (that.width_inc.is_set())
        width_inc = that.width_inc;
    if (that.height_inc.is_set())
        height_inc = that.height_inc;
    if (that.min_aspect.is_set())
        min_aspect = that.min_aspect;
    if (that.max_aspect.is_set())
        max_aspect = that.max_aspect;
    if (that.streams.is_set())
        streams = that.streams;
    if (that.parent.is_set())
        parent = that.parent;
    if (that.input_shape.is_set())
        input_shape = that.input_shape;
    if (that.input_mode.is_set())
        input_mode = that.input_mode;
    if (that.shell_chrome.is_set())
        shell_chrome = that.shell_chrome;
    if (that.confine_pointer.is_set())
        confine_pointer = that.confine_pointer;
    if (that.cursor_image.is_set())
        cursor_image = that.cursor_image;
    if (that.depth_layer.is_set())
        depth_layer = that.depth_layer;
    if (that.attached_edges.is_set())
        attached_edges = that.attached_edges;
    if (that.exclusive_rect.is_set())
        exclusive_rect = that.exclusive_rect;
    if (that.ignore_exclusion_zones.is_set())
        ignore_exclusion_zones = that.ignore_exclusion_zones;
    if (that.application_id.is_set())
        application_id = that.application_id;
    if (that.server_side_decorated.is_set())
        server_side_decorated = that.server_side_decorated;
    if (that.focus_mode.is_set())
        focus_mode = that.focus_mode;
    if (that.visible_on_lock_screen.is_set())
        visible_on_lock_screen = that.visible_on_lock_screen;
    if (that.tiled_edges.is_set())
        tiled_edges = that.tiled_edges;
    if (that.alpha.is_set())
        alpha = that.alpha;
    if (that.parent_size.is_set())
        parent_size = that.parent_size;
}

bool msh::operator==(
    const msh::SurfaceSpecification& lhs,
    const msh::SurfaceSpecification& rhs)
{
    auto const parents_equal =
        (!lhs.parent && !rhs.parent) ||
        (lhs.parent && rhs.parent && lhs.parent.value().lock() == rhs.parent.value().lock());

    return parents_equal &&
        std::tie(lhs.name, lhs.width, lhs.height, lhs.top_left,
                 lhs.buffer_usage, lhs.pixel_format, lhs.output_id,
                 lhs.state, lhs.type, lhs.preferred_orientation,
                 lhs.parent_id, lhs.aux_rect, lhs.edge_attachment,
                 lhs.placement_hints, lhs.surface_placement_gravity,
                 lhs.aux_rect_placement_gravity,
                 lhs.aux_rect_placement_offset_x, lhs.aux_rect_placement_offset_y,
                 lhs.min_width, lhs.min_height, lhs.max_width, lhs.max_height,
                 lhs.width_inc, lhs.height_inc, lhs.min_aspect, lhs.max_aspect,
                 lhs.input_shape, lhs.input_mode, lhs.shell_chrome, lhs.streams,
                 lhs.confine_pointer, lhs.cursor_image, lhs.depth_layer,
                 lhs.attached_edges, lhs.exclusive_rect, lhs.ignore_exclusion_zones,
                 lhs.application_id, lhs.server_side_decorated, lhs.focus_mode,
                 lhs.visible_on_lock_screen, lhs.tiled_edges, lhs.alpha, lhs.parent_size) ==
        std::tie(rhs.name, rhs.width, rhs.height, rhs.top_left,
                 rhs.buffer_usage, rhs.pixel_format, rhs.output_id,
                 rhs.state, rhs.type, rhs.preferred_orientation,
                 rhs.parent_id, rhs.aux_rect, rhs.edge_attachment,
                 rhs.placement_hints, rhs.surface_placement_gravity,
                 rhs.aux_rect_placement_gravity,
                 rhs.aux_rect_placement_offset_x, rhs.aux_rect_placement_offset_y,
                 rhs.min_width, rhs.min_height, rhs.max_width, rhs.max_height,
                 rhs.width_inc, rhs.height_inc, rhs.min_aspect, rhs.max_aspect,
                 rhs.input_shape, rhs.input_mode, rhs.shell_chrome, rhs.streams,
                 rhs.confine_pointer, rhs.cursor_image, rhs.depth_layer,
                 rhs.attached_edges, rhs.exclusive_rect, rhs.ignore_exclusion_zones,
                 rhs.application_id, rhs.server_side_decorated, rhs.focus_mode,
                 rhs.visible_on_lock_screen, rhs.tiled_edges, rhs.alpha, rhs.parent_size);
}
