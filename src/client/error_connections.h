/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#ifndef MIR_CLIENT_ERROR_CONNECTIONS_H_
#define MIR_CLIENT_ERROR_CONNECTIONS_H_

#include "mir_toolkit/client_types.h"
#include <mutex>
#include <unordered_set>

namespace mir
{
namespace client
{

class ErrorConnections
{
public:
    static ErrorConnections& instance();

    void insert(MirConnection* connection);
    void remove(MirConnection* connection);
    bool contains(MirConnection* connection) const;

private:
    ErrorConnections() = default;
    ErrorConnections(ErrorConnections const&) = delete;
    ErrorConnections& operator=(ErrorConnections const&) = delete;

    mutable std::mutex connection_guard;
    std::unordered_set<MirConnection*> connections;
};

}
}

#endif /* MIR_CLIENT_ERROR_CONNECTIONS_H_ */
