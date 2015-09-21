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

#include "mir_toolkit/version.h"

namespace mir
{
namespace protobuf
{
// For the present we use the client API protocol_version as a proxy for the protocol
// version as the protocol is typically updated to support API changes.
// If we need to break protocol without a corresponding ABI break then we need to bump "epoch"
inline constexpr int protocol_version(int major, int minor, int epoch = 0)
{
    return MIR_VERSION_NUMBER(epoch, major, minor);
}

inline constexpr int current_protocol_version()
{ 
    return protocol_version(MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION); 
}

// Client libraries older than last ABI break may make calls we can't understand
inline constexpr int oldest_compatible_protocol_version()
{
    return protocol_version(MIR_CLIENT_MAJOR_VERSION, 0);
}
}
}

#endif //MIR_PROTOBUF_PROTOCOL_VERSION_H
