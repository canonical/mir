/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "mir/graphics/nested/nested_mir_connection_handle.h"
#include "mir_toolkit/mir_client_library.h"

namespace mgn = mir::graphics::nested;

mgn::MirConnectionHandle::MirConnectionHandle(MirConnection* const mir_connection)
    : connection(mir_connection)
{
}

mgn::MirConnectionHandle::~MirConnectionHandle()
{
    mir_connection_release(connection);
}
