/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_PLATFORMS_COMMON_EGL_HELPERS_H_
#define MIR_PLATFORMS_COMMON_EGL_HELPERS_H_

#include <EGL/egl.h>

namespace mir::graphics::common
{

/// Utility to restore EGL state at construction on scope exit
class CacheEglState
{
public:
    CacheEglState();
    ~CacheEglState();

    CacheEglState(CacheEglState&& from);
    auto operator=(CacheEglState&& rhs) -> CacheEglState&;
private:
    CacheEglState(CacheEglState const&) = delete;
    EGLDisplay dpy;
    EGLContext ctx;
    EGLSurface draw_surf;
    EGLSurface read_surf;
};
}

#endif // MIR_PLATFORMS_COMMON_EGL_HELPERS_H_
