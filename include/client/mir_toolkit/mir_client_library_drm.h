/*
 * Copyright © 2012 Canonical Ltd.
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
 */

#ifndef MIR_CLIENT_LIBRARY_DRM_H_
#define MIR_CLIENT_LIBRARY_DRM_H_

#include "mir_toolkit/mir_client_library.h"

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

struct gbm_device;

typedef void (*mir_drm_auth_magic_callback)(int status, void *context);

/* Authenticates a DRM magic cookie */
MirWaitHandle *mir_connection_drm_auth_magic(MirConnection *connection,
                                             unsigned int magic,
                                             mir_drm_auth_magic_callback callback,
                                             void *context);

/**
 * Set the gbm_device to be used by the EGL implementation.
 * This is required if the application needs to create EGLImages from
 * gbm buffers objects created on that gbm device.
 * \param [in] connection  The connection
 * \param [in] dev         The gbm_device to set
 * \return                 A non-zero value if the operation was successful,
 *                         0 otherwise
 */
int mir_connection_drm_set_gbm_device(MirConnection* connection,
                                      struct gbm_device* dev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_CLIENT_LIBRARY_DRM_H_ */
