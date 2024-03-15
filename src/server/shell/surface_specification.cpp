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

#include "mir/shell/surface_specification.h"

namespace msh = mir::shell;

auto msh::operator==(SurfaceAspectRatio const& lhs, SurfaceAspectRatio const& rhs) -> bool
{
    return
        lhs.width == rhs.width &&
        lhs.height == rhs.height;
}

auto msh::operator==(StreamSpecification const& lhs, StreamSpecification const& rhs) -> bool
{
    return
        lhs.stream.lock() == rhs.stream.lock() &&
        lhs.displacement == rhs.displacement &&
        lhs.size == rhs.size;
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
        !visible_on_lock_screen.is_set();
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
}

bool msh::operator==(
    const msh::SurfaceSpecification& lhs,
    const msh::SurfaceSpecification& rhs)
{
    return
        lhs.name == rhs.name &&
        lhs.width == rhs.width &&
        lhs.height == rhs.height &&
        lhs.top_left == rhs.top_left &&
        lhs.buffer_usage == rhs.buffer_usage &&
        lhs.pixel_format == rhs.pixel_format &&
        lhs.output_id == rhs.output_id &&
        lhs.state == rhs.state &&
        lhs.type == rhs.type &&
        lhs.preferred_orientation == rhs.preferred_orientation &&
        lhs.parent_id == rhs.parent_id &&
        lhs.aux_rect == rhs.aux_rect &&
        lhs.edge_attachment == rhs.edge_attachment &&
        lhs.placement_hints == rhs.placement_hints &&
        lhs.surface_placement_gravity == rhs.surface_placement_gravity &&
        lhs.aux_rect_placement_gravity == rhs.aux_rect_placement_gravity &&
        lhs.aux_rect_placement_offset_x == rhs.aux_rect_placement_offset_x &&
        lhs.aux_rect_placement_offset_y == rhs.aux_rect_placement_offset_y &&
        (   // parent check passes if neither have a parent, or if they both have parents and they're equal
            (!lhs.parent && !rhs.parent) ||
            (lhs.parent && rhs.parent && lhs.parent.value().lock() == rhs.parent.value().lock())
        ) &&
        lhs.min_width == rhs.min_width &&
        lhs.min_height == rhs.min_height &&
        lhs.max_width == rhs.max_width &&
        lhs.max_height == rhs.max_height &&
        lhs.width_inc == rhs.width_inc &&
        lhs.height_inc == rhs.height_inc &&
        lhs.min_aspect == rhs.min_aspect &&
        lhs.max_aspect == rhs.max_aspect &&
        lhs.input_shape == rhs.input_shape &&
        lhs.input_mode == rhs.input_mode &&
        lhs.shell_chrome == rhs.shell_chrome &&
        lhs.streams == rhs.streams &&
        lhs.confine_pointer == rhs.confine_pointer &&
        lhs.cursor_image == rhs.cursor_image &&
        lhs.depth_layer == rhs.depth_layer &&
        lhs.attached_edges == rhs.attached_edges &&
        lhs.exclusive_rect == rhs.exclusive_rect &&
        lhs.ignore_exclusion_zones == rhs.ignore_exclusion_zones &&
        lhs.application_id == rhs.application_id &&
        lhs.server_side_decorated == rhs.server_side_decorated &&
        lhs.focus_mode == rhs.focus_mode &&
        lhs.visible_on_lock_screen == rhs.visible_on_lock_screen;
}

bool msh::operator!=(
    const msh::SurfaceSpecification& lhs,
    const msh::SurfaceSpecification& rhs)
{
    return !(lhs == rhs);
}
