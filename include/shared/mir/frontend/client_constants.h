/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_CLIENT_CONSTANTS_H_
#define MIR_FRONTEND_CLIENT_CONSTANTS_H_

namespace mir
{

namespace frontend
{
/// Number of buffers the client library will keep.

/// mir::client::ClientBufferDepository and mir::frontend::ClientBufferTracker need to use the same value
unsigned int const client_buffer_cache_size = 3;

/// Buffers need to be big enough to support messages
unsigned int const serialization_buffer_size = 2048;
}
}

#endif
