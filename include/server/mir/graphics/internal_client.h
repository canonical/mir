/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */


#ifndef MIR_GRAPHICS_INTERNAL_CLIENT_H_
#define MIR_GRAPHICS_INTERNAL_CLIENT_H_

#include <EGL/egl.h>

#include <memory>

namespace mir
{
namespace graphics
{
class InternalSurface;

/**
 * Interface to in-process client support.
 */
class InternalClient
{
public:
    /**
     * The EGL native display to be used by in-process clients.
     */
    virtual EGLNativeDisplayType egl_native_display() = 0;

    /**
     * The EGL native window to be used by in-process clients for a surface.
     */
    virtual EGLNativeWindowType egl_native_window(std::shared_ptr<InternalSurface> const&) = 0;

protected:
    InternalClient() = default;
    virtual ~InternalClient() = default;
    InternalClient(InternalClient const&) = delete;
    InternalClient& operator=(InternalClient const&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_INTERNAL_CLIENT_H_ */
