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

#ifndef MIR_SHELL_SURFACE_SPECIFICATION_H_
#define MIR_SHELL_SURFACE_SPECIFICATION_H_

#include <mir/flags.h>
#include <optional>
#include <mir_toolkit/common.h>
#include <mir/frontend/surface_id.h>
#include <mir/geometry/point.h>
#include <mir/geometry/displacement.h>
#include <mir/graphics/buffer_properties.h>
#include <mir/graphics/display_configuration.h>

#include <string>
#include <memory>

namespace mir
{
namespace graphics { class CursorImage; }
namespace scene { class Surface; }
namespace input { enum class InputReceptionMode; }
namespace frontend
{
class BufferStream;
}

namespace shell
{
struct SurfaceAspectRatio
{
    unsigned width;
    unsigned height;

    bool operator==(SurfaceAspectRatio const&) const = default;
};

struct StreamSpecification
{
    std::weak_ptr<frontend::BufferStream> stream;
    geometry::Displacement displacement;
};
auto operator==(StreamSpecification const& lhs, StreamSpecification const& rhs) -> bool;

/// Specification of surface properties requested by client
struct SurfaceSpecification
{
    bool is_empty() const;
    void update_from(SurfaceSpecification const& that);
    /// Sets width and height at the same time
    void set_size(geometry::Size size) { width = size.width; height = size.height; }

    std::optional<geometry::Point> top_left;
    std::optional<geometry::Width> width;
    std::optional<geometry::Height> height;
    std::optional<MirPixelFormat> pixel_format;
    std::optional<graphics::BufferUsage> buffer_usage;
    std::optional<std::string> name;
    std::optional<graphics::DisplayConfigurationOutputId> output_id;
    std::optional<MirWindowType> type;
    std::optional<MirWindowState> state;
    std::optional<MirOrientationMode> preferred_orientation;
    std::optional<frontend::SurfaceId> parent_id;
    std::optional<geometry::Rectangle> aux_rect;
    std::optional<MirEdgeAttachment> edge_attachment;
    std::optional<MirPlacementHints> placement_hints;
    std::optional<MirPlacementGravity> surface_placement_gravity;
    std::optional<MirPlacementGravity> aux_rect_placement_gravity;
    std::optional<int> aux_rect_placement_offset_x;
    std::optional<int> aux_rect_placement_offset_y;
    /// The expected size of the parent surface at the time of popup placement.
    /// Used to constrain popup placement when the parent is being resized.
    std::optional<geometry::Size> parent_size;
    std::optional<geometry::Width> min_width;
    std::optional<geometry::Height> min_height;
    std::optional<geometry::Width> max_width;
    std::optional<geometry::Height> max_height;
    std::optional<geometry::DeltaX> width_inc;
    std::optional<geometry::DeltaY> height_inc;
    std::optional<SurfaceAspectRatio> min_aspect;
    std::optional<SurfaceAspectRatio> max_aspect;
    std::optional<std::vector<StreamSpecification>> streams;
    std::optional<std::weak_ptr<scene::Surface>> parent;

    std::optional<std::vector<geometry::Rectangle>> input_shape;
    std::optional<input::InputReceptionMode> input_mode;

    std::optional<MirShellChrome> shell_chrome;
    std::optional<MirPointerConfinementState> confine_pointer;
    std::optional<std::shared_ptr<graphics::CursorImage>> cursor_image;

    /// Child surfaces are by default created on the same layer as their parent, and updating the depth layer of a parent
    /// also updates all children.
    std::optional<MirDepthLayer> depth_layer;
    std::optional<bool> visible_on_lock_screen;

    std::optional<MirPlacementGravity> attached_edges;
    std::optional<std::optional<geometry::Rectangle>> exclusive_rect;
    std::optional<bool> ignore_exclusion_zones;
    std::optional<std::string> application_id;

    /// If to enable server-side decorations for this surface
    std::optional<bool> server_side_decorated;

    /// How the surface should gain and lose focus
    std::optional<MirFocusMode> focus_mode;

    /// Describes which edges are touching part of the tiling grid
    std::optional<Flags<MirTiledEdge>> tiled_edges;

    /// The opacity of the window.
    std::optional<float> alpha;
};
bool operator==(SurfaceSpecification const& lhs, SurfaceSpecification const& rhs);
}
}

#endif /* MIR_SHELL_SURFACE_SPECIFICATION_H_ */
