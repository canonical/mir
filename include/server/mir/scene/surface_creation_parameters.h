/*
 * Copyright © 2013-2016 Canonical Ltd.
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

#ifndef MIR_SCENE_SURFACE_CREATION_PARAMETERS_H_
#define MIR_SCENE_SURFACE_CREATION_PARAMETERS_H_

#include "mir_toolkit/common.h"
#include "mir/geometry/point.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/display_configuration.h"
#include "mir/frontend/surface_id.h"
#include "mir/input/input_reception_mode.h"
#include "mir/optional_value.h"
#include "mir/shell/surface_specification.h"

#include <memory>
#include <string>

namespace mir
{
namespace shell
{
struct SurfaceSpecification;
}
namespace compositor
{
class BufferStream;
}
namespace scene
{
class Surface;

struct SurfaceCreationParameters
{
    SurfaceCreationParameters();

    SurfaceCreationParameters& of_name(std::string const& new_name);

    SurfaceCreationParameters& of_size(geometry::Size new_size);

    SurfaceCreationParameters& of_size(geometry::Width::ValueType width, geometry::Height::ValueType height);

    SurfaceCreationParameters& of_position(geometry::Point const& top_left);

    SurfaceCreationParameters& of_buffer_usage(graphics::BufferUsage new_buffer_usage);

    SurfaceCreationParameters& of_pixel_format(MirPixelFormat new_pixel_format);

    SurfaceCreationParameters& with_input_mode(input::InputReceptionMode const& new_mode);

    SurfaceCreationParameters& with_output_id(graphics::DisplayConfigurationOutputId const& output_id);

    SurfaceCreationParameters& of_type(MirWindowType type);

    SurfaceCreationParameters& with_state(MirWindowState state);

    SurfaceCreationParameters& with_preferred_orientation(MirOrientationMode mode);

    SurfaceCreationParameters& with_parent_id(frontend::SurfaceId const& id);

    SurfaceCreationParameters& with_aux_rect(geometry::Rectangle const& rect);

    SurfaceCreationParameters& with_edge_attachment(MirEdgeAttachment edge);

    SurfaceCreationParameters& with_buffer_stream(std::shared_ptr<compositor::BufferStream> const& stream);

    void update_from(shell::SurfaceSpecification const& that);

    std::string name;
    geometry::Size size;
    geometry::Point top_left;
    graphics::BufferUsage buffer_usage;
    MirPixelFormat pixel_format;
    input::InputReceptionMode input_mode;
    graphics::DisplayConfigurationOutputId output_id;

    mir::optional_value<MirWindowState> state;
    mir::optional_value<MirWindowType> type;
    mir::optional_value<MirOrientationMode> preferred_orientation;
    mir::optional_value<frontend::SurfaceId> parent_id;
    std::weak_ptr<frontend::BufferStream> content;
    mir::optional_value<geometry::Rectangle> aux_rect;
    mir::optional_value<MirEdgeAttachment> edge_attachment;
    optional_value<MirPlacementHints> placement_hints;
    optional_value<MirPlacementGravity> surface_placement_gravity;
    optional_value<MirPlacementGravity> aux_rect_placement_gravity;
    optional_value<int> aux_rect_placement_offset_x;
    optional_value<int> aux_rect_placement_offset_y;

    std::weak_ptr<Surface> parent;

    optional_value<geometry::Width> min_width;
    optional_value<geometry::Height> min_height;
    optional_value<geometry::Width> max_width;
    optional_value<geometry::Height> max_height;
    mir::optional_value<geometry::DeltaX> width_inc;
    mir::optional_value<geometry::DeltaY> height_inc;
    mir::optional_value<shell::SurfaceAspectRatio> min_aspect;
    mir::optional_value<shell::SurfaceAspectRatio> max_aspect;

    mir::optional_value<std::vector<geometry::Rectangle>> input_shape;
    mir::optional_value<MirShellChrome> shell_chrome;
    mir::optional_value<std::vector<shell::StreamSpecification>> streams;
    mir::optional_value<MirPointerConfinementState> confine_pointer;

    /// If the depth layer of a child surface isn't set, it gets the layer of its parent
    optional_value<MirDepthLayer> depth_layer;

    /// The edge of the output to attach this surface to
    /// Only used if the surface is in state mir_window_state_attached
    optional_value<MirPlacementGravity> attached_edges;

    /// The area of this surface that will not be occluded
    /// Only used if surface is in state mir_window_state_attached and is attached to an edge (not a corner)
    optional_value<geometry::Rectangle> exclusive_rect;

    /// See mir::scene::Surface::application_id()
    optional_value<std::string> application_id;

    /// If to enable server-side decorations for this surface
    optional_value<bool> server_side_decorated;
};

bool operator==(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);
bool operator!=(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);

SurfaceCreationParameters a_surface();
}
}

#endif /* MIR_SCENE_SURFACE_CREATION_PARAMETERS_H_ */
