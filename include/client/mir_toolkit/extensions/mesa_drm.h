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

#ifndef MIR_CLIENT_EXTENSIONS_MESA_DRM_H_
#define MIR_CLIENT_EXTENSIONS_MESA_DRM_H_

#define MIR_EXTENSION_MESA_DRM "2782f477-bb9c-473b-a9ed-e4e85c61c0d1"
#define MIR_EXTENSION_MESA_DRM_VERSION_1 1

#ifdef __cplusplus
extern "C" {
#endif

struct MirConnection;
typedef void (*mir_auth_fd_callback)(
    int auth_fd, void* context);
typedef void (*mir_extension_mesa_drm_auth_fd)(MirConnection*, mir_auth_fd_callback cb, void* context);


struct MirExtensionMesaDRM
{
    mir_extension_mesa_drm_auth_fd drm_auth_fd;
};

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_MESA_DRM_H_ */
