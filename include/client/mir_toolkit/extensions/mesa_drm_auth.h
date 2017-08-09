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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_MESA_DRM_AUTH_H_
#define MIR_CLIENT_EXTENSIONS_MESA_DRM_AUTH_H_

#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

struct MirConnection;

typedef void (*MirAuthFdCallback)(
    int auth_fd, void* context);
/*
 * Request authenticated FD from server.
 * \param [in] connection   The connection
 * \param [in] cb           The callback triggered on server response
 * \param [in] context      The context for the callback
 */
typedef void (*MirExtensionMesaDrmAuthFd)(MirConnection*, MirAuthFdCallback cb, void* context);

typedef void (*MirAuthMagicCallback)(
    int response, void* context);
/*
 * Request magic cookie from server.
 * \param [in] connection   The connection
 * \param [in] magic        The magic
 * \param [in] cb           The callback triggered on server response
 * \param [in] context      The context for the callback
 */
typedef void (*MirExtensionMesaDrmAuthMagic)(
    MirConnection* connection, int magic, MirAuthMagicCallback cb, void* context);

typedef struct MirExtensionMesaDRMAuthV1
{
    MirExtensionMesaDrmAuthFd drm_auth_fd;
    MirExtensionMesaDrmAuthMagic drm_auth_magic;
} MirExtensionMesaDRMAuthV1;

static inline MirExtensionMesaDRMAuthV1 const* mir_extension_mesa_drm_auth_v1(
    MirConnection* connection)
{
    return (MirExtensionMesaDRMAuthV1 const*) mir_connection_request_extension(
        connection, "mir_extension_mesa_drm_auth", 1);
}

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_MESA_DRM_AUTH_H_ */
