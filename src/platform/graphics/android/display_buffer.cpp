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
      current_configuration{mg::DisplayConfigurationOutputId{1},
                            mg::DisplayConfigurationCardId{0},
                            mg::DisplayConfigurationOutputType::lvds,
                            {
                                fb_bundle->fb_format()
                            },
                            {mg::DisplayConfigurationMode{fb_bundle->fb_size(),0.0f}},
                            0,
                            geom::Size{0,0},
                            true,
                            true,
                            geom::Point{0,0},
                            0,
                            fb_bundle->fb_format(),
                            mir_power_mode_on,
                            mir_orientation_normal}
{
}

geom::Rectangle mga::DisplayBuffer::view_area() const
{
    auto const& size = fb_bundle->fb_size();
    int width = size.width.as_int();
    int height = size.height.as_int();

    if (current_configuration.orientation == mir_orientation_left
        || current_configuration.orientation == mir_orientation_right)
    {
        std::swap(width, height);
    }

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
        std::list<Renderable> const& renderlist,
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
        render_fn(renderable);
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
    return current_configuration.orientation;
}

std::shared_ptr<mg::DisplayConfigurationOutput> mga::DisplayBuffer::configuration() const
{
    return std::make_shared<mg::DisplayConfigurationOutput>(current_configuration);
}

void mga::DisplayBuffer::configure(DisplayConfigurationOutput const& new_configuration)
{
    MirPowerMode intended_power_mode = new_configuration.power_mode;
    if ((intended_power_mode == mir_power_mode_standby) ||
        (intended_power_mode == mir_power_mode_suspend))
    {
        intended_power_mode = mir_power_mode_off;
    }

    if (intended_power_mode != current_configuration.power_mode)
    {
        display_device->mode(intended_power_mode);
    }

    //apply orientation
    if (display_device->apply_orientation(new_configuration.orientation))
    {
        //hardware is taking care of orienting for us
        current_configuration.orientation = mir_orientation_normal;
    }
    else
    {
        //GL compositor will rotate
        current_configuration.orientation = new_configuration.orientation;
    }
}
