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

#include "miral/window_specification.h"
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

    mir::optional_value<Point> top_left;
    mir::optional_value<Size> size;
    mir::optional_value<MirPixelFormat> pixel_format;
    mir::optional_value<BufferUsage> buffer_usage;
    mir::optional_value<std::string> name;
    mir::optional_value<int> output_id;
    mir::optional_value<MirWindowType> type;
    mir::optional_value<MirWindowState> state;
    mir::optional_value<MirOrientationMode> preferred_orientation;
    mir::optional_value<Rectangle> aux_rect;
    mir::optional_value<MirPlacementHints> placement_hints;
    mir::optional_value<MirPlacementGravity> window_placement_gravity;
    mir::optional_value<MirPlacementGravity> aux_rect_placement_gravity;
    mir::optional_value<Displacement> aux_rect_placement_offset;
    mir::optional_value<Width> min_width;
    mir::optional_value<Height> min_height;
    mir::optional_value<Width> max_width;
    mir::optional_value<Height> max_height;
    mir::optional_value<DeltaX> width_inc;
    mir::optional_value<DeltaY> height_inc;
    mir::optional_value<AspectRatio> min_aspect;
    mir::optional_value<AspectRatio> max_aspect;
    mir::optional_value<std::vector<mir::shell::StreamSpecification>> streams;
    mir::optional_value<std::weak_ptr<mir::scene::Surface>> parent;
    mir::optional_value<std::vector<Rectangle>> input_shape;
    mir::optional_value<InputReceptionMode> input_mode;
    mir::optional_value<MirShellChrome> shell_chrome;
    mir::optional_value<MirPointerConfinementState> confine_pointer;
    mir::optional_value<MirDepthLayer> depth_layer;
    mir::optional_value<MirPlacementGravity> attached_edges;
    mir::optional_value<mir::optional_value<mir::geometry::Rectangle>> exclusive_rect;
    mir::optional_value<bool> ignore_exclusion_zones;
    mir::optional_value<std::string> application_id;
    mir::optional_value<bool> server_side_decorated;
    mir::optional_value<MirFocusMode> focus_mode;
    mir::optional_value<bool> visible_on_lock_screen;
    mir::optional_value<std::shared_ptr<void>> userdata;
};

auto make_surface_spec(WindowSpecification const& miral_spec) -> mir::shell::SurfaceSpecification;

}

#endif  // MIR_WINDOW_SPECIFICATION_INTERNAL_H
