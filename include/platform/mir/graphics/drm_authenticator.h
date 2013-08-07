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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_DRM_AUTHENTICATOR_H_
#define MIR_GRAPHICS_DRM_AUTHENTICATOR_H_

#include <xf86drm.h>

namespace mir
{
namespace graphics
{

class DRMAuthenticator
{
public:
    virtual ~DRMAuthenticator() {}

    virtual void drm_auth_magic(drm_magic_t magic) = 0;

protected:
    DRMAuthenticator() = default;
    DRMAuthenticator(const DRMAuthenticator&) = delete;
    DRMAuthenticator& operator=(const DRMAuthenticator&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_DRM_AUTHENTICATOR_H_ */
