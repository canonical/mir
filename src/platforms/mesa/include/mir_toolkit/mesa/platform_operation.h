/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TOOLKIT_MESA_PLATFORM_OPERATION_H_
#define MIR_TOOLKIT_MESA_PLATFORM_OPERATION_H_

#ifdef __cplusplus
/**
 *  \addtogroup mir_toolkit
 *  @{
 */
extern "C"
{
#endif

/*
 * Supported platform operations for the Mesa driver
 */

enum MirMesaPlatformOperation
{
    auth_magic = 1,
    auth_fd = 2,
    set_gbm_device = 3
};

/*
 * MesaPlatformOperation::auth_magic related structures
 */

struct MirMesaAuthMagicRequest
{
    unsigned int magic;
};

struct MirMesaAuthMagicResponse
{
    int status; /* 0 on success, a positive error number on failure */
};

/*
 * MesaPlatformOperation::set_gbm_device related structures
 */

struct gbm_device;

struct MirMesaSetGBMDeviceRequest
{
    struct gbm_device* device;
};

struct MirMesaSetGBMDeviceResponse
{
    int status; /* 0 on success, a positive error number on failure */
};

#ifdef __cplusplus
}
/**@}*/
#endif

#endif
