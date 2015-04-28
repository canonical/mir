/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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

#include <string>

namespace mir
{
namespace client
{
class ClientBufferStream;
}
}

// Parameters for configuring the apperance and behavior of the system cursor. 
// Will grow to include cursors specified by raw RGBA data, hotspots, etc...
struct MirCursorConfiguration 
{
    MirCursorConfiguration(char const* name);
    MirCursorConfiguration(mir::client::ClientBufferStream const* stream, int hotspot_x, int hotspot_y);

    std::string name;
    mir::client::ClientBufferStream const* stream;
    int hotspot_x;
    int hotspot_y;
};

#endif // MIR_CLIENT_CURSOR_CONFIGURATION_H_
