/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/surface_specification.h"
#include "mir/compositor/buffer_stream.h"

namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace geom = mir::geometry;

ms::SurfaceCreationParameters::SurfaceCreationParameters()
    : buffer_usage{mg::BufferUsage::undefined},
      pixel_format{mir_pixel_format_invalid},
      input_mode{mi::InputReceptionMode::normal}
{
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_name(std::string const& new_name)
{
    name = new_name;
    return *this;
}


ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_size(
        geometry::Size new_size)
{
    size = new_size;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_size(
    geometry::Width::ValueType width,
    geometry::Height::ValueType height)
{
    return of_size({width, height});
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_position(geometry::Point const& new_top_left)
{
    top_left = new_top_left;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_buffer_usage(
        mg::BufferUsage new_buffer_usage)
{
    buffer_usage = new_buffer_usage;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_pixel_format(
    MirPixelFormat new_pixel_format)
{
    pixel_format = new_pixel_format;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_input_mode(input::InputReceptionMode const& new_mode)
{
    input_mode = new_mode;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_output_id(
    graphics::DisplayConfigurationOutputId const& new_output_id)
{
    output_id = new_output_id;

    return *this;
}


ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_type(MirWindowType the_type)
{
    type = the_type;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_state(MirWindowState the_state)
{
    state = the_state;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_preferred_orientation(MirOrientationMode mode)
{
    preferred_orientation = mode;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_parent_id(mf::SurfaceId const& id)
{
    parent_id = id;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_aux_rect(geometry::Rectangle const& rect)
{
    aux_rect = rect;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_edge_attachment(MirEdgeAttachment edge)
{
    edge_attachment = edge;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_buffer_stream(
    std::shared_ptr<compositor::BufferStream> const& stream)
{
    content = stream;
    return *this;
}

void ms::SurfaceCreationParameters::update_from(msh::SurfaceSpecification const& that)
{
    if (that.width.is_set() && that.height.is_set())
        size = geom::Size{that.width.value(), that.height.value()};
    if (that.pixel_format.is_set())
        pixel_format = that.pixel_format.value();
    if (that.buffer_usage.is_set())
        buffer_usage = that.buffer_usage.value();
    if (that.name.is_set())
        name = that.name.value();
    if (that.output_id.is_set())
        output_id = that.output_id.value();
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
        parent = that.parent.value();
    if (that.input_shape.is_set())
        input_shape = that.input_shape;
    if (that.shell_chrome.is_set())
        shell_chrome = that.shell_chrome;
    if (that.confine_pointer.is_set())
        confine_pointer = that.confine_pointer;
    if (that.depth_layer.is_set())
        depth_layer = that.depth_layer;
    if (that.attached_edges.is_set())
        attached_edges = that.attached_edges;
    if (that.exclusive_rect.is_set())
        exclusive_rect = that.exclusive_rect.value();
    if (that.application_id.is_set())
        application_id = that.application_id.value();
    // server_side_decorated not a property of SurfaceSpecification
    // TODO: should SurfaceCreationParameters support cursors?
//     if (that.cursor_image.is_set())
//         cursor_image = that.cursor_image;
//     if (that.stream_cursor.is_set())
//         stream_cursor = that.stream_cursor;
}

bool ms::operator==(
    const SurfaceCreationParameters& lhs,
    const SurfaceCreationParameters& rhs)
{
    return
        lhs.name == rhs.name &&
        lhs.size == rhs.size &&
        lhs.top_left == rhs.top_left &&
        lhs.buffer_usage == rhs.buffer_usage &&
        lhs.pixel_format == rhs.pixel_format &&
        lhs.input_mode == rhs.input_mode &&
        lhs.output_id == rhs.output_id &&
        lhs.state == rhs.state &&
        lhs.type == rhs.type &&
        lhs.preferred_orientation == rhs.preferred_orientation &&
        lhs.parent_id == rhs.parent_id &&
        lhs.content.lock() == rhs.content.lock() &&
        lhs.aux_rect == rhs.aux_rect &&
        lhs.edge_attachment == rhs.edge_attachment &&
        lhs.placement_hints == rhs.placement_hints &&
        lhs.surface_placement_gravity == rhs.surface_placement_gravity &&
        lhs.aux_rect_placement_gravity == rhs.aux_rect_placement_gravity &&
        lhs.aux_rect_placement_offset_x == rhs.aux_rect_placement_offset_x &&
        lhs.aux_rect_placement_offset_y == rhs.aux_rect_placement_offset_y &&
        lhs.parent.lock() == rhs.parent.lock() &&
        lhs.min_width == rhs.min_width &&
        lhs.min_height == rhs.min_height &&
        lhs.max_width == rhs.max_width &&
        lhs.max_height == rhs.max_height &&
        lhs.width_inc == rhs.width_inc &&
        lhs.height_inc == rhs.height_inc &&
        lhs.min_aspect == rhs.min_aspect &&
        lhs.max_aspect == rhs.max_aspect &&
        lhs.input_shape == rhs.input_shape &&
        lhs.shell_chrome == rhs.shell_chrome &&
        lhs.streams == rhs.streams &&
        lhs.confine_pointer == rhs.confine_pointer &&
        lhs.depth_layer == rhs.depth_layer &&
        lhs.attached_edges == rhs.attached_edges &&
        lhs.exclusive_rect == rhs.exclusive_rect &&
        lhs.application_id == rhs.application_id &&
        lhs.server_side_decorated == rhs.server_side_decorated;
}

bool ms::operator!=(
    const SurfaceCreationParameters& lhs,
    const SurfaceCreationParameters& rhs)
{
    return !(lhs == rhs);
}

ms::SurfaceCreationParameters ms::a_surface()
{
    return SurfaceCreationParameters();
}
