/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "error_connections.h"

namespace mcl = mir::client;

mcl::ErrorConnections& mcl::ErrorConnections::instance()
{
    static ErrorConnections* error_connections = new ErrorConnections();
    return *error_connections;
}

void mcl::ErrorConnections::insert(MirConnection* connection)
{
    std::lock_guard<std::mutex> lock(connection_guard);
    connections.insert(connection);
}

void mcl::ErrorConnections::remove(MirConnection* connection)
{
    std::lock_guard<std::mutex> lock(connection_guard);
    connections.erase(connection);
}

bool mcl::ErrorConnections::contains(MirConnection* connection) const
{
    std::lock_guard<std::mutex> lock(connection_guard);
    return connections.count(connection);
}
