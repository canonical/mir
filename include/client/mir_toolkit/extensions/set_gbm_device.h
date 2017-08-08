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

#ifndef MIR_CLIENT_EXTENSIONS_SET_GBM_DEVICE_H_
#define MIR_CLIENT_EXTENSIONS_SET_GBM_DEVICE_H_

#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gbm_device;

//Set the gbm device used by the client
//  \param [in] device    The gbm_device.
//  \param [in] context   The context to set the gbm device.
typedef void (*MirSetGbmDevice)(struct gbm_device*, void* const context);
typedef struct MirExtensionSetGbmDeviceV1
{
    MirSetGbmDevice set_gbm_device;
    void* const context;
} MirExtensionSetGbmDeviceV1;

//legacy compatibility
typedef MirExtensionSetGbmDeviceV1 MirExtensionSetGbmDevice;

static inline MirExtensionSetGbmDeviceV1 const* mir_extension_set_gbm_device_v1(
    MirConnection* connection)
{
    return (MirExtensionSetGbmDeviceV1 const*) mir_connection_request_extension(
        connection, "mir_extension_set_gbm_device", 1);
}

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_SET_GBM_DEVICE_H_ */
