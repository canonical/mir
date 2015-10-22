/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_PROTOBUF_PROTOCOL_VERSION_H
#define MIR_PROTOBUF_PROTOCOL_VERSION_H

#include "mir_toolkit/mir_version_number.h"

namespace mir
{
namespace protobuf
{
inline constexpr int current_protocol_version()
{ 
    return MIR_VERSION_NUMBER(0,3,1);
}

inline constexpr int oldest_compatible_protocol_version()
{
    return MIR_VERSION_NUMBER(0,3,0);
}

inline constexpr int next_incompatible_protocol_version()
{
    return MIR_VERSION_NUMBER(1,0,0);
}
}
}

#endif //MIR_PROTOBUF_PROTOCOL_VERSION_H
