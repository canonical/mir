/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com
 */

#include "mir/shell/surface_specification.h"

namespace msh = mir::shell;

bool msh::SurfaceSpecification::is_empty() const
{
    // You know, compile-time reflection would be pretty
    // useful here :)
    return !width.is_set() &&
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
        !shell_chrome.is_set() &&
        !depth_layer.is_set();
}

void msh::SurfaceSpecification::update_from(SurfaceSpecification const& that)
{
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
    if (that.shell_chrome.is_set())
        shell_chrome = that.shell_chrome;
    if (that.confine_pointer.is_set())
        confine_pointer = that.confine_pointer;
    if (that.cursor_image.is_set())
        cursor_image = that.cursor_image;
    if (that.stream_cursor.is_set())
        stream_cursor = that.stream_cursor;
    if (that.depth_layer.is_set())
        depth_layer = that.depth_layer;
}
