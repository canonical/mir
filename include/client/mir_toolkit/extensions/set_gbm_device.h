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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_SET_GBM_DEVICE_H_
#define MIR_CLIENT_EXTENSIONS_SET_GBM_DEVICE_H_

#define MIR_EXTENSION_SET_GBM_DEVICE "6d38bfa6-1257-4b31-8e6d-910e7769093d"
#define MIR_EXTENSION_SET_GBM_DEVICE_VERSION_1 1

#ifdef __cplusplus
extern "C" {
#endif

struct gbm_device;

//Set the gbm device used by the client
//  \param [in] device    The gbm_device, or nullptr on failure. Sets errno on failure.
//  \param [context] device    The gbm_device, or nullptr on failure. Sets errno on failure.
typedef void (*_set_gbm_device)(struct gbm_device*, void* const context);
struct MirExtensionSetGbmDevice
{
    _set_gbm_device set_gbm_device;
    void* const context;
};

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_SET_GBM_DEVICE_H_ */
