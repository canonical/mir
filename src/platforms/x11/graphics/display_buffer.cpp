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
                                  xcb_window_t win,
                                  geometry::Rectangle const& view_area,
                                  geometry::Size const& window_size,
                                  EGLContext const shared_context,
                                  std::shared_ptr<DisplayReport> const& r,
                                  GLConfig const& gl_config)
                                  : report{r},
                                    area{view_area},
                                    window_size{window_size},
                                    transform(1),
                                    egl{gl_config, x_dpy, win, shared_context},
                                    output_id{output_id}
{
    egl.report_egl_configuration(
        [&r] (EGLDisplay disp, EGLConfig cfg)
        {
            r->report_egl_configuration(disp, cfg);
        });
}

geom::Rectangle mgx::DisplayBuffer::view_area() const
{
    return area;
}

auto mgx::DisplayBuffer::size() const -> geom::Size
{
    return window_size;
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

void mgx::DisplayBuffer::set_size(geom::Size const& size)
{
    window_size = size;
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
