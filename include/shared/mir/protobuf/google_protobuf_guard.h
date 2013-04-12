/*
 * Copyright © 2012 Canonical Ltd.
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

#ifndef MIR_PROTOBUF_GOOGLE_PROTOBUF_GUARD_H_
#define MIR_PROTOBUF_GOOGLE_PROTOBUF_GUARD_H_

namespace mir
{

/// subsystem dealing with Google protobuf protocol.
namespace protobuf
{
void google_protobuf_guard();
}
}

// Any translation unit that includes this header will get this as part
// of its initialization, and, in turn, this ensures that protobuf gets
// initialized (and cleaned up) in a sensible sequence.
namespace
{
bool force_google_protobuf_init{(mir::protobuf::google_protobuf_guard(), true)};
}

#endif /* MIR_PROTOBUF_GOOGLE_PROTOBUF_GUARD_H_ */
