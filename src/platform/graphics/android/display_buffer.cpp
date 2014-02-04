/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "framebuffer_bundle.h"
#include "display_buffer.h"
#include "display_device.h"

#include <functional>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::DisplayBuffer::DisplayBuffer(
    std::shared_ptr<FramebufferBundle> const& fb_bundle,
    std::shared_ptr<DisplayDevice> const& display_device,
    std::shared_ptr<ANativeWindow> const& native_window,
    mga::GLContext const& shared_gl_context)
    : fb_bundle{fb_bundle},
      display_device{display_device},
      native_window{native_window},
      gl_context{shared_gl_context, std::bind(mga::create_window_surface, std::placeholders::_1, std::placeholders::_2, native_window.get())},
      rotation{mir_orientation_normal}
{
}

geom::Rectangle mga::DisplayBuffer::view_area() const
{
    auto const& size = fb_bundle->fb_size();
    int width = size.width.as_int();
    int height = size.height.as_int();

    if (rotation == mir_orientation_left || rotation == mir_orientation_right)
        std::swap(width, height);

    return {{0,0}, {width,height}};
}

void mga::DisplayBuffer::make_current()
{
    gl_context.make_current();
}

void mga::DisplayBuffer::release_current()
{
    gl_context.release_current();
}

void mga::DisplayBuffer::render_and_post_update(
        std::list<std::shared_ptr<Renderable>> const& renderlist,
        std::function<void(Renderable const&)> const& render_fn)
{
    if (renderlist.empty())
    {
        display_device->prepare_gl();
    }
    else
    {
        display_device->prepare_gl_and_overlays(renderlist);
    }

    for(auto& renderable : renderlist)
    {
        render_fn(*renderable);
    }

    render_and_post();
}

void mga::DisplayBuffer::post_update()
{
    display_device->prepare_gl();
    render_and_post();
}

void mga::DisplayBuffer::render_and_post()
{
    display_device->gpu_render(gl_context.display(), gl_context.surface());
    auto last_rendered = fb_bundle->last_rendered_buffer();
    display_device->post(*last_rendered);
}

bool mga::DisplayBuffer::can_bypass() const
{
    return false;
}

MirOrientation mga::DisplayBuffer::orientation() const
{
    /*
     * android::DisplayBuffer is aways created with physical width/height
     * (not rotated). So we just need to pass through the desired rotation
     * and let the renderer do it.
     * If and when we choose to implement HWC rotation, this may change.
     */
    return rotation;
}

void mga::DisplayBuffer::orient(MirOrientation rot)
{
    rotation = rot;
}
