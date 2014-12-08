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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_DRM_AUTHENTICATION_H_
#define MIR_GRAPHICS_MESA_DRM_AUTHENTICATION_H_

#include <xf86drm.h>
#include "mir/fd.h"

namespace mir
{
namespace graphics
{
namespace mesa
{
class DRMAuthentication
{
public:
    DRMAuthentication() = default;
    virtual ~DRMAuthentication() = default;
    DRMAuthentication(DRMAuthentication const&) = delete;
    DRMAuthentication& operator=(DRMAuthentication const&) = delete;

    virtual void auth_magic(drm_magic_t magic) = 0;
    virtual mir::Fd authenticated_fd() = 0;
};
}
}
}

#endif /* MIR_GRAPHICS_MESA_DRM_AUTHENTICATION_H_ */
