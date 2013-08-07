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

#ifndef MIR_GRAPHICS_NESTED_MIR_CONNECTION_HANDLE_H_
#define MIR_GRAPHICS_NESTED_MIR_CONNECTION_HANDLE_H_

struct MirConnection;

namespace mir
{
namespace graphics
{
namespace nested
{

class MirConnectionHandle
{
public:
    MirConnectionHandle(MirConnection* const mir_connection);
    ~MirConnectionHandle();

    MirConnectionHandle(MirConnectionHandle const&) = delete;
    MirConnectionHandle& operator=(MirConnectionHandle const& connection_handle) = delete;

    operator MirConnection*() {return (MirConnection*)connection;}

private:
    MirConnection* const connection;
};

}
}
}
#endif // MIR_GRAPHICS_NESTED_MIR_CONNECTION_HANDLE_H_
