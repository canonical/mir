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
 */

#ifndef MIR_COMMON_CLIENT_VISIBLE_ERROR_H_
#define MIR_COMMON_CLIENT_VISIBLE_ERROR_H_

#include "mir_toolkit/client_types.h"

#include <exception>
#include <stdexcept>

namespace mir
{

/**
 * Base class for exceptions which might be visible to clients
 *
 * When a mir::ClientVisibleError exception is propagated to the server IPC boundary
 * it is translated into a client-side MirError and sent to the callback registered
 * with mir_connection_set_error_callback().
 */
class ClientVisibleError : public std::runtime_error
{
public:
    ClientVisibleError(std::string const& description)
        : std::runtime_error(description)
    {
    }

    /**
     * Client-visible error domain
     */
    virtual MirErrorDomain domain() const noexcept = 0;
    /**
     * Error code within the domain().
     */
    virtual uint32_t code() const noexcept = 0;
};
}

#endif //MIR_COMMON_CLIENT_VISIBLE_ERROR_H_
