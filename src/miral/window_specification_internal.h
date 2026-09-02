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

#ifndef MIR_WINDOW_SPECIFICATION_INTERNAL_H
#define MIR_WINDOW_SPECIFICATION_INTERNAL_H

#include <miral/window_specification.h>
#include <mir/shell/surface_specification.h>

namespace miral
{

struct WindowSpecification::Self
{
    enum class BufferUsage
    {
        undefined,
        /** rendering using GL */
        hardware,
        /** rendering using direct pixel access */
        software
    };

    Self() = default;
    Self(Self const&) = default;
    Self(mir::shell::SurfaceSpecification const& spec);

    std::optional<Point> top_left;
    std::optional<Size> size;
    std::optional<MirPixelFormat> pixel_format;
    std::optional<BufferUsage> buffer_usage;
    std::optional<std::string> name;
    std::optional<int> output_id;
    std::optional<MirWindowType> type;
    std::optional<MirWindowState> state;
    std::optional<MirOrientationMode> preferred_orientation;
    std::optional<Rectangle> aux_rect;
    std::optional<MirPlacementHints> placement_hints;
    std::optional<MirPlacementGravity> window_placement_gravity;
    std::optional<MirPlacementGravity> aux_rect_placement_gravity;
    std::optional<Displacement> aux_rect_placement_offset;
    std::optional<Width> min_width;
    std::optional<Height> min_height;
    std::optional<Width> max_width;
    std::optional<Height> max_height;
    std::optional<DeltaX> width_inc;
    std::optional<DeltaY> height_inc;
    std::optional<AspectRatio> min_aspect;
    std::optional<AspectRatio> max_aspect;
    std::optional<std::vector<mir::shell::StreamSpecification>> streams;
    std::optional<std::weak_ptr<mir::scene::Surface>> parent;
    std::optional<std::vector<Rectangle>> input_shape;
    std::optional<InputReceptionMode> input_mode;
    std::optional<MirShellChrome> shell_chrome;
    std::optional<MirPointerConfinementState> confine_pointer;
    std::optional<MirDepthLayer> depth_layer;
    std::optional<MirPlacementGravity> attached_edges;
    std::optional<std::optional<mir::geometry::Rectangle>> exclusive_rect;
    std::optional<bool> ignore_exclusion_zones;
    std::optional<std::string> application_id;
    std::optional<bool> server_side_decorated;
    std::optional<MirFocusMode> focus_mode;
    std::optional<bool> visible_on_lock_screen;
    std::optional<mir::Flags<MirTiledEdge>> tiled_edges;
    std::optional<std::shared_ptr<void>> userdata;
    std::optional<float> opacity;
    std::optional<Size> parent_size;
};

auto make_surface_spec(WindowSpecification const& miral_spec) -> mir::shell::SurfaceSpecification;

}

#endif  // MIR_WINDOW_SPECIFICATION_INTERNAL_H
