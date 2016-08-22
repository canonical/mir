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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "mir/graphics/egl_error.h"
#include "mir/graphics/atomic_frame.h"
#include "display_buffer.h"
#include <cstring>
#include <boost/throw_exception.hpp>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::DisplayBuffer::DisplayBuffer(geom::Size const sz,
                                  EGLDisplay const d,
                                  EGLSurface const s,
                                  EGLContext const c,
                                  std::shared_ptr<AtomicFrame> const& f,
                                  MirOrientation const o)
                                  : size{sz},
                                    egl_dpy{d},
                                    egl_surf{s},
                                    egl_ctx{c},
                                    last_frame{f},
                                    orientation_{o}
{
    /*
     * EGL_CHROMIUM_sync_control is an EGL extension that Google invented/copied
     * to replace GLX:
     *    https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
     * Most noteworthy is that the EGL extension only has one function, as
     * Google realized that's all you need. You do not need wait functions or
     * events if you already have accurate timestamps and the ability to sleep
     * with high precision.
     *
     * EGL_CHROMIUM_sync_control never got formally standardized. Google
     * faced resistance from NVIDIA who pointed out that it hinders correct
     * operation of the G-Sync adaptive frame rate technology (as well as AMD's
     * FreeSync).
     *
     * Eventually Google stopped trying to standardize EGL_CHROMIUM_sync_control
     * when they switched ChromeOS over from X11 to Freon, which does the same
     * thing but with native KMS.
     *
     * History and spec:
     *    https://bugs.chromium.org/p/chromium/issues/detail?id=366935
     */
    auto extensions = eglQueryString(egl_dpy, EGL_EXTENSIONS);
    eglGetSyncValues =
        reinterpret_cast<EglGetSyncValuesCHROMIUM*>(
            strstr(extensions, "EGL_CHROMIUM_sync_control") ?
            eglGetProcAddress("eglGetSyncValuesCHROMIUM") : NULL
            );
}

geom::Rectangle mgx::DisplayBuffer::view_area() const
{
    switch (orientation_)
    {
    case mir_orientation_left:
    case mir_orientation_right:
        return {{0,0}, {size.height.as_int(), size.width.as_int()}};
    default:
        return {{0,0}, size};
    }
}

void mgx::DisplayBuffer::make_current()
{
    if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot make current"));
}

void mgx::DisplayBuffer::release_current()
{
    if (!eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot make uncurrent"));
}

bool mgx::DisplayBuffer::post_renderables_if_optimizable(RenderableList const& /*renderlist*/)
{
    return false;
}

void mgx::DisplayBuffer::swap_buffers()
{
    if (!eglSwapBuffers(egl_dpy, egl_surf))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot swap"));

    Frame frame;
    if (eglGetSyncValues) // We allow for this to be missing because calling
    {                     // it may also fail, which needs handling too...
        int64_t ust, msc, sbc;
        if (eglGetSyncValues(egl_dpy, egl_surf, &ust, &msc, &sbc))
        {
            Frame prev = last_frame->load();
            frame.msc = msc;
            frame.clock_id = CLOCK_MONOTONIC;
            frame.ust = ust;
            frame.min_ust_interval = (prev.msc && prev.ust && msc > prev.msc) ?
                                     (ust - prev.ust)/(msc - prev.msc) :
                                     prev.min_ust_interval;
        }
    }
    last_frame->store(frame);
}

void mgx::DisplayBuffer::bind()
{
}

MirOrientation mgx::DisplayBuffer::orientation() const
{
    return orientation_;
}

MirMirrorMode mgx::DisplayBuffer::mirror_mode() const
{
    return mir_mirror_mode_none;
}

void mgx::DisplayBuffer::set_orientation(MirOrientation const new_orientation)
{
    orientation_ = new_orientation;
}

mg::NativeDisplayBuffer* mgx::DisplayBuffer::native_display_buffer()
{
    return this;
}
