/*
 * Copyright © 2020 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_WINDOW_INFO_INTERNAL_H
#define MIR_WINDOW_INFO_INTERNAL_H

#include "miral/window_info.h"

struct miral::WindowInfo::Self
{
    Self(Window window, WindowSpecification const& params);
    Self();

    Window window;
    std::string name;
    MirWindowType type;
    MirWindowState state;
    mir::geometry::Rectangle restore_rect;
    Window parent;
    std::vector <Window> children;
    mir::geometry::Width min_width;
    mir::geometry::Height min_height;
    mir::geometry::Width max_width;
    mir::geometry::Height max_height;
    MirOrientationMode preferred_orientation;
    MirPointerConfinementState confine_pointer;

    mir::geometry::DeltaX width_inc;
    mir::geometry::DeltaY height_inc;
    AspectRatio min_aspect;
    AspectRatio max_aspect;
    mir::optional_value<int> output_id;
    MirShellChrome shell_chrome;
    MirDepthLayer depth_layer;
    MirPlacementGravity attached_edges;
    mir::optional_value<mir::geometry::Rectangle> exclusive_rect;
    std::shared_ptr<void> userdata;
};

#endif  // MIR_WINDOW_INFO_INTERNAL_H
