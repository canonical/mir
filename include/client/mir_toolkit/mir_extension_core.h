/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_CLIENT_EXTENSION_CORE_H_
#define MIR_CLIENT_EXTENSION_CORE_H_

#include "mir_toolkit/mir_connection.h"

/**
 * Request an extension interface thingy.
 *
 * \param [in] connection
 * \param [in] interface
 * \param [in] version
 * \return
 */
void* mir_connection_request_interface(
    MirConnection* connection,
    char const* interface,
    int version);

#endif //MIR_CLIENT_EXTENSION_CORE_H_
