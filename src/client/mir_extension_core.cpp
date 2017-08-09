/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_extension_core.h"
#include "mir_connection.h"
#include "mir/uncaught.h"
#include "mir/require.h"

void const* mir_connection_request_extension(
    MirConnection* connection,
    char const* interface,
    int version)
try
{
    mir::require(connection);
    return const_cast<void const*>(connection->request_interface(interface, version));
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}
