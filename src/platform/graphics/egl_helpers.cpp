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

#include <mir/graphics/egl_helpers.h>
#include <utility>

namespace mgc = mir::graphics::common;

mgc::CacheEglState::CacheEglState()
    : dpy{eglGetCurrentDisplay()},
      ctx{eglGetCurrentContext()},
      draw_surf{eglGetCurrentSurface(EGL_DRAW)},
      read_surf{eglGetCurrentSurface(EGL_READ)}
{
}

mgc::CacheEglState::CacheEglState(CacheEglState&& from)
    : dpy{std::exchange(from.dpy, EGL_NO_DISPLAY)},
      ctx{std::exchange(from.ctx, EGL_NO_CONTEXT)},
      draw_surf{std::exchange(from.draw_surf, EGL_NO_SURFACE)},
      read_surf{std::exchange(from.read_surf, EGL_NO_SURFACE)}
{
}

auto mgc::CacheEglState::operator=(CacheEglState&& rhs) -> CacheEglState&
{
    dpy = std::exchange(rhs.dpy, EGL_NO_DISPLAY);
    ctx = std::exchange(rhs.ctx, EGL_NO_CONTEXT);
    draw_surf = std::exchange(rhs.draw_surf, EGL_NO_SURFACE);
    read_surf = std::exchange(rhs.read_surf, EGL_NO_SURFACE);
    return *this;
}

mgc::CacheEglState::~CacheEglState()
{
    if (dpy != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(dpy, draw_surf, read_surf, ctx);
    }
}
