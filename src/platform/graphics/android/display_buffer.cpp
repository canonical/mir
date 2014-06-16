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
#include <sstream>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::DisplayBuffer::DisplayBuffer(
    std::shared_ptr<FramebufferBundle> const& fb_bundle,
    std::shared_ptr<DisplayDevice> const& display_device,
    std::shared_ptr<ANativeWindow> const& native_window,
    mga::GLContext const& shared_gl_context,
    mg::GLProgramFactory const& program_factory,
    mga::OverlayOptimization overlay_option)
    : fb_bundle{fb_bundle},
      display_device{display_device},
      native_window{native_window},
      gl_context{shared_gl_context, fb_bundle, native_window},
      overlay_program{program_factory, gl_context, geom::Rectangle{{0,0},fb_bundle->fb_size()}},
      overlay_enabled{overlay_option == mga::OverlayOptimization::enabled},
      current_configuration{
          mg::DisplayConfigurationOutputId{1},
          mg::DisplayConfigurationCardId{0},
          mg::DisplayConfigurationOutputType::lvds,
          {
              fb_bundle->fb_format()
          },
          {mg::DisplayConfigurationMode{fb_bundle->fb_size(), fb_bundle->fb_refresh_rate()}},
          0,
          geom::Size{0,0}, //could use DPI information to fill this
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

bool mga::DisplayBuffer::post_renderables_if_optimizable(RenderableList const& renderlist)
{
    if (!overlay_enabled)
        return false;

    return display_device->post_overlays(gl_context, renderlist, overlay_program);
}

void mga::DisplayBuffer::post_update()
{
    display_device->post_gl(gl_context);
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

bool mga::DisplayBuffer::uses_alpha() const
{
    return false;
}

mg::DisplayConfigurationOutput mga::DisplayBuffer::configuration() const
{
    return mg::DisplayConfigurationOutput(current_configuration);
}

void mga::DisplayBuffer::configure(DisplayConfigurationOutput const& new_configuration)
{
    //power mode
    MirPowerMode intended_power_mode = new_configuration.power_mode;
    if ((intended_power_mode == mir_power_mode_standby) ||
        (intended_power_mode == mir_power_mode_suspend))
    {
        intended_power_mode = mir_power_mode_off;
    }

    if (intended_power_mode != current_configuration.power_mode)
    {
        display_device->mode(intended_power_mode);
        current_configuration.power_mode = intended_power_mode;
    }

    //If the hardware can rotate for us, we report normal orientation. If it can't
    //we preserve this orientation change so the compositor can rotate everything in GL 
    if (display_device->apply_orientation(new_configuration.orientation))
    {
        current_configuration.orientation = mir_orientation_normal;
    }
    else
    {
        current_configuration.orientation = new_configuration.orientation;
    }

    //do not allow fb format reallocation
    if (new_configuration.current_format != current_configuration.current_format)
    {
        std::stringstream err_msg; 
        err_msg << std::string("could not change display buffer format to request: ")
                << new_configuration.current_format;
        BOOST_THROW_EXCEPTION(std::runtime_error(err_msg.str()));
    }
}
