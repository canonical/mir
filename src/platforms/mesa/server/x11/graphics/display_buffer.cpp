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
#include "display_buffer.h"
#include "atomic_frame.h"
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
#if 0
    // TODO: Replace this
    set_frame_callback([](Frame const& frame)
    {
        unsigned long long frame_seq = frame.msc;
        unsigned long long frame_usec = frame.ust;
        static unsigned long long prev = frame.ust;
        struct timespec now;
        clock_gettime(frame.clock_id, &now);
        unsigned long long now_usec = now.tv_sec*1000000ULL +
                                      now.tv_nsec/1000;
        long long age_usec = now_usec - frame_usec;
        fprintf(stderr, "TODO - Frame #%llu at %llu.%06llus (%lldus ago) delta %lldus\n",
                        frame_seq,
                        frame_usec/1000000ULL, frame_usec%1000000ULL,
                        age_usec, frame_usec - prev);
        prev = frame_usec;
    });
#endif

    /*
     * EGL_CHROMIUM_sync_control is an unofficial extension and simplification
     * that Google copied from:
     *    https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
     * as a way to free Chromium from depending on GLX. And Mesa implements it
     * for EGL on X11 only.
     *
     * EGL_CHROMIUM_sync_control never got formally standardized. Google
     * faced resistance from NVIDIA who pointed out that it hinders correct
     * operation of the G-Sync adaptive frame rate technology (as well as AMD's
     * FreeSync).
     *
     * Eventually Google stopped trying to standardize EGL_CHROMIUM_sync_control
     * when they switched ChromeOS over from X11 to Freon. In Freon they instead
     * now use native KMS, the same way as our mesa-kms driver. In doing so
     * they have not resolved NVIDIA's complaints, just avoided them.
     *
     * History and semi-official spec:
     *    https://bugs.chromium.org/p/chromium/issues/detail?id=366935
     *
     * TLDR: We need this, it's the only existing solution for EGL on X11 and
     *       it still works until further notice...
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
        uint64_t ust, msc, sbc;
        if (eglGetSyncValues(egl_dpy, egl_surf, &ust, &msc, &sbc))
        {
            auto delta = msc - frame.msc;
            if (delta)
            {
                frame.msc = msc;
                // Always monotonic? The Chromium source suggests no. But the
                // libdrm source says you can only find out with drmGetCap :(
                // This appears to be correct for all modern systems though...
                frame.clock_id = CLOCK_MONOTONIC;
                frame.ust = ust;
                // TODO: Get the correct frame.min_ust_interval
                frame.min_ust_interval = 1000000/60;
            }
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
