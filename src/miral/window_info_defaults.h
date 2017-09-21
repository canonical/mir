/*
 * Copyright © 2017 Canonical Ltd.
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

#ifndef MIR_WINDOW_INFO_DEFAULTS_H
#define MIR_WINDOW_INFO_DEFAULTS_H

#include "miral/window_info.h"

namespace miral
{
Width  extern const default_min_width;
Height extern const default_min_height;
Width  extern const default_max_width;
Height extern const default_max_height;
DeltaX extern const default_width_inc;
DeltaY extern const default_height_inc;
WindowInfo::AspectRatio extern const default_min_aspect_ratio;
WindowInfo::AspectRatio extern const default_max_aspect_ratio;
}

#endif //MIR_WINDOW_INFO_DEFAULTS_H
