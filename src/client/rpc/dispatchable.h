/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_CLIENT_RPC_DISPATCHABLE_H_
#define MIR_CLIENT_RPC_DISPATCHABLE_H_

#include "mir/fd.h"

namespace mir
{
namespace client
{
namespace rpc
{
class Dispatchable
{
public:
    Dispatchable() = default;
    virtual ~Dispatchable() = default;

    Dispatchable& operator=(Dispatchable const&) = delete;
    Dispatchable(Dispatchable const&) = delete;

    /**
     * \brief Get a poll()able file descriptor
     * \return A file descriptor usable with poll() or equivalent function calls that
     *         becomes readable when there are dispatchable events
     */
    virtual Fd watch_fd() const = 0;

    /**
     * \brief Dispatch one pending event
     */
    virtual void dispatch() = 0;
};
}
}
}

#endif // MIR_CLIENT_RPC_DISPATCHABLE_H_
