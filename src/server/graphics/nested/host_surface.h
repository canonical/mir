/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_HOST_SURFACE_H_
#define MIR_GRAPHICS_NESTED_HOST_SURFACE_H_

#include "mir_toolkit/client_types.h"

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace nested
{

class HostSurface
{
public:
    virtual ~HostSurface() = default;

    virtual EGLNativeWindowType egl_native_window() = 0;
    virtual void set_event_handler(mir_surface_event_callback cb,
                                   void* context) = 0;
protected:
    HostSurface() = default;
    HostSurface(HostSurface const&) = delete;
    HostSurface& operator=(HostSurface const&) = delete;
};

}
}
}
#endif // MIR_GRAPHICS_NESTED_HOST_SURFACE_H_
