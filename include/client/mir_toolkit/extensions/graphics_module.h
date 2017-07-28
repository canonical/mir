/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_GRAPHICS_MODULE_H_
#define MIR_CLIENT_EXTENSIONS_GRAPHICS_MODULE_H_

#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Query graphics platform module.
 *
 * \note The char pointers in MirModuleProperties are owned by the connection and should not be
 * freed. They remain valid until the connection is released.
 *
 *   \param [in]  connection    The connection
 *   \param [out] properties    Structure to be populated
 */
typedef void (*MirConnectionGetGraphicsModule)
    (MirConnection *connection, MirModuleProperties *properties);

typedef struct MirExtensionGraphicsModuleV1
{
    MirConnectionGetGraphicsModule graphics_module;
} MirExtensionGraphicsModuleV1;

static inline MirExtensionGraphicsModuleV1 const* mir_extension_graphics_module_v1(
    MirConnection* connection)
{
    return (MirExtensionGraphicsModuleV1 const*) mir_connection_request_extension(
        connection, "mir_extension_graphics_module", 1);
}

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_GRAPHICS_MODULE_H_ */
