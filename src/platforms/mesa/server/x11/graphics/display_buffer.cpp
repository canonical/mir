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

#include "mir/fatal.h"
#include "display_buffer.h"
#include "display_configuration.h"
#include "mir/graphics/display_report.h"

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::DisplayBuffer::DisplayBuffer(::Display* const x_dpy,
                                  Window const win,
                                  geom::Size const sz,
                                  EGLContext const shared_context,
                                  std::shared_ptr<DisplayReport> const& r,
                                  MirOrientation const o,
                                  GLConfig const& gl_config)
                                  : size{sz},
                                    report{r},
                                    orientation_{o},
                                    egl{gl_config}
{
    egl.setup(x_dpy, win, shared_context);
    egl.report_egl_configuration(
        [&r] (EGLDisplay disp, EGLConfig cfg)
        {
            r->report_egl_configuration(disp, cfg);
        });
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
     * Admittedly we are not a real display and will miss some real vsyncs
     * but this is best-effort. And besides, we don't want Mir reporting all
     * real vsyncs because that would mean the compositor never sleeps.
     */
    report->report_vsync(mgx::DisplayConfiguration::the_output_id.as_value());
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
