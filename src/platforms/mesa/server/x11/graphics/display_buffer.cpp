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
#include "display_configuration.h"
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
                                  std::shared_ptr<DisplayReport> const& r,
                                  MirOrientation const o)
                                  : size{sz},
                                    egl_dpy{d},
                                    egl_surf{s},
                                    egl_ctx{c},
                                    last_frame{f},
                                    report{r},
                                    orientation_{o}
{
    /*
     * EGL_CHROMIUM_sync_control is an EGL extension that Google invented/copied
     * so they could switch Chrome(ium) from GLX to EGL:
     *
     *    https://bugs.chromium.org/p/chromium/issues/detail?id=366935
     *    https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
     *
     * Most noteworthy is that the EGL extension only has one function, as
     * Google realized that's all you need. You do not need wait functions or
     * events if you already have accurate timestamps and the ability to sleep
     * with high precision. In fact sync logic in clients will have higher
     * precision if you implement the wait yourself relative to the correct
     * kernel clock, than using IPC to implement the wait on the server.
     *
     * EGL_CHROMIUM_sync_control never got formally standardized and no longer
     * needs to be since they switched ChromeOS over to Freon (native KMS).
     * However this remains the correct and only way of doing it in EGL on X11.
     * AFAIK the only existing implementation is Mesa.
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

bool mgx::DisplayBuffer::overlay(RenderableList const& /*renderlist*/)
{
    return false;
}

void mgx::DisplayBuffer::swap_buffers()
{
    if (!eglSwapBuffers(egl_dpy, egl_surf))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot swap"));

    /*
     * It would be nice to call this on demand as required. However the
     * implementation requires an EGL context. So for simplicity we call it here
     * on every frame.
     *   This does mean the current last_frame will be a bit out of date if
     * the compositor missed a frame. But that doesn't actually matter because
     * the consequence of that would be the client scheduling the next frame
     * immediately without waiting, which is probably ideal anyway.
     */
    int64_t ust_us, msc, sbc;
    if (eglGetSyncValues &&
        eglGetSyncValues(egl_dpy, egl_surf, &ust_us, &msc, &sbc))
    {
        std::chrono::nanoseconds const ust_ns{ust_us * 1000};
        mg::Frame frame;
        frame.msc = msc;
        frame.ust = {CLOCK_MONOTONIC, ust_ns};
        last_frame->store(frame);
        (void)sbc; // unused
    }
    else  // Extension not available? Fall back to a reasonable estimate:
    {
        last_frame->increment_now();
    }

    /*
     * Admittedly we are not a real display and will miss some real vsyncs
     * but this is best-effort. And besides, we don't want Mir reporting all
     * real vsyncs because that would mean the compositor never sleeps.
     */
    report->report_vsync(mgx::DisplayConfiguration::the_output_id.as_value(),
                         last_frame->load());
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
