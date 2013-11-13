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

#include "display_buffer.h"
#include "display_device.h"

#include <functional>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::DisplayBuffer::DisplayBuffer(std::shared_ptr<DisplayDevice> const& display_device,
    std::shared_ptr<ANativeWindow> const& native_window,
    mga::GLContext const& shared_gl_context)
    : display_device{display_device},
      native_window{native_window},
      gl_context{shared_gl_context, std::bind(mga::create_window_surface, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, native_window.get())}
{
}

geom::Rectangle mga::DisplayBuffer::view_area() const
{
    return {geom::Point{}, display_device->display_size()};
}

void mga::DisplayBuffer::make_current()
{
    gl_context.make_current();
}

void mga::DisplayBuffer::release_current()
{
    gl_context.release_current();
}

void mga::DisplayBuffer::post_update()
{
#if 0
    display_device->prepare();

    if (display_device->should_swapbuffers())
        gl_context.swap_buffers();

    auto fb = framebuffers->last_rendered();
    display_device->commit_frame(fb);
#endif
}

bool mga::DisplayBuffer::can_bypass() const
{
    return false;
}
