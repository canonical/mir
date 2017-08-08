/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CLIENT_CURSOR_CONFIGURATION_H_
#define MIR_CLIENT_CURSOR_CONFIGURATION_H_

#include "mir/mir_buffer_stream.h"
#include <string>

// Parameters for configuring the apperance and behavior of the system cursor. 
// Will grow to include cursors specified by raw RGBA data, hotspots, etc...
struct MirCursorConfiguration 
{
    MirCursorConfiguration(char const* name);
    MirCursorConfiguration(MirBufferStream const* stream, int hotspot_x, int hotspot_y);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirCursorConfiguration(MirRenderSurface const* surface, int hotspot_x, int hotspot_y);
#pragma GCC diagnostic pop

    std::string name;
    MirBufferStream const* stream;
    int hotspot_x;
    int hotspot_y;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirRenderSurface const* surface;
#pragma GCC diagnostic pop
};

#endif // MIR_CLIENT_CURSOR_CONFIGURATION_H_
