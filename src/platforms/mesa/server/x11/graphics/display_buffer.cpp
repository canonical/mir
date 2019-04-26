/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "mir/graphics/atomic_frame.h"
#include "mir/fatal.h"
#include "display_buffer.h"
#include "display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/transformation.h"
#include <cstring>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::DisplayBuffer::DisplayBuffer(::Display* const x_dpy,
                                  DisplayConfigurationOutputId output_id,
                                  Window const win,
                                  geometry::Size const& view_area_size,
                                  EGLContext const shared_context,
                                  std::shared_ptr<AtomicFrame> const& f,
                                  std::shared_ptr<DisplayReport> const& r,
                                  GLConfig const& gl_config)
                                  : report{r},
                                    area{{0,0},view_area_size},
                                    transform(1),
                                    egl{gl_config},
                                    last_frame{f},
                                    output_id{output_id},
                                    eglGetSyncValues{nullptr}
{
    egl.setup(x_dpy, win, shared_context);
    egl.report_egl_configuration(
        [&r] (EGLDisplay disp, EGLConfig cfg)
        {
            r->report_egl_configuration(disp, cfg);
        });

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
    auto const extensions = eglQueryString(egl.display(), EGL_EXTENSIONS);
    auto const extension = "EGL_CHROMIUM_sync_control";
    auto const found = strstr(extensions, extension);
    if (found)
    {
        char end = found[strlen(extension)];
        if (end == '\0' || end == ' ')
            eglGetSyncValues = reinterpret_cast<EglGetSyncValuesCHROMIUM*>(
                                 eglGetProcAddress("eglGetSyncValuesCHROMIUM"));
    }
}

geom::Rectangle mgx::DisplayBuffer::view_area() const
{
    return area;
}

void mgx::DisplayBuffer::make_current()
{
    if (!egl.make_current())
        fatal_error("Failed to make EGL surface current");
}

void mgx::DisplayBuffer::release_current()
{
    egl.release_current();
}

bool mgx::DisplayBuffer::overlay(RenderableList const& /*renderlist*/)
{
    return false;
}

void mgx::DisplayBuffer::swap_buffers()
{
    if (!egl.swap_buffers())
        fatal_error("Failed to perform buffer swap");

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
        eglGetSyncValues(egl.display(), egl.surface(), &ust_us, &msc, &sbc))
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
    report->report_vsync(output_id.as_value(), last_frame->load());
}

void mgx::DisplayBuffer::bind()
{
}

glm::mat2 mgx::DisplayBuffer::transformation() const
{
    return transform;
}

void mgx::DisplayBuffer::set_view_area(geom::Rectangle const& a)
{
    area = a;
}

void mgx::DisplayBuffer::set_transformation(glm::mat2 const& t)
{
    transform = t;
}

mg::NativeDisplayBuffer* mgx::DisplayBuffer::native_display_buffer()
{
    return this;
}

void mgx::DisplayBuffer::for_each_display_buffer(
    std::function<void(graphics::DisplayBuffer&)> const& f)
{
    f(*this);
}

void mgx::DisplayBuffer::post()
{
}

std::chrono::milliseconds mgx::DisplayBuffer::recommended_sleep() const
{
    return std::chrono::milliseconds::zero();
}
