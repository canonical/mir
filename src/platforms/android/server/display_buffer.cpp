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

#include "mir/graphics/transformation.h"
#include "framebuffer_bundle.h"
#include "display_buffer.h"
#include "display_device.h"
#include "hwc_layerlist.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace mg=mir::graphics;
namespace mgl=mir::gl;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::DisplayBuffer::DisplayBuffer(
    mga::DisplayName display_name,
    std::unique_ptr<LayerList> layer_list,
    std::shared_ptr<FramebufferBundle> const& fb_bundle,
    std::shared_ptr<DisplayDevice> const& display_device,
    std::shared_ptr<ANativeWindow> const& native_window,
    mga::GLContext const& shared_gl_context,
    mgl::ProgramFactory const& program_factory,
    glm::mat2 const& transform,
    geom::Rectangle area,
    mga::OverlayOptimization overlay_option)
    : display_name(display_name),
      layer_list(std::move(layer_list)),
      fb_bundle{fb_bundle},
      display_device{display_device},
      native_window{native_window},
      gl_context{shared_gl_context, fb_bundle, native_window},
      overlay_program{program_factory, gl_context, geom::Rectangle{{0,0},fb_bundle->fb_size()}},
      overlay_enabled{overlay_option == mga::OverlayOptimization::enabled},
      transform{transform},
      area{area},
      power_mode_{mir_power_mode_on}
{
}

geom::Rectangle mga::DisplayBuffer::view_area() const
{
    return area;
}

void mga::DisplayBuffer::make_current()
{
    gl_context.make_current();
}

void mga::DisplayBuffer::release_current()
{
    gl_context.release_current();
}

bool mga::DisplayBuffer::overlay(RenderableList const& renderlist)
{
    glm::mat2 static const no_transformation;
    if (!overlay_enabled ||
        !display_device->compatible_renderlist(renderlist) ||
        transform != no_transformation)
        return false;

    layer_list->update_list(renderlist, area.top_left - geom::Point());

    bool needs_commit{false};
    for (auto& layer : *layer_list)
        needs_commit |= layer.needs_commit;

    return needs_commit;
}

void mga::DisplayBuffer::swap_buffers()
{
    layer_list->update_list({}, area.top_left - geom::Point());
    //HWC 1.0 cannot call eglSwapBuffers() on the display context
    if (display_device->can_swap_buffers())
        gl_context.swap_buffers();
}

void mga::DisplayBuffer::bind()
{
}

glm::mat2 mga::DisplayBuffer::transformation() const
{
    return transform;
}

void mga::DisplayBuffer::configure(MirPowerMode power_mode,
                                   glm::mat2 const& trans,
                                   geom::Rectangle const& a)
{
    power_mode_ = power_mode;
    area = a;
    if (power_mode_ != mir_power_mode_on)
        display_device->content_cleared();
    transform = trans;
}

mga::DisplayContents mga::DisplayBuffer::contents()
{
    return mga::DisplayContents{display_name, *layer_list, area.top_left - geom::Point(), gl_context, overlay_program};
}

MirPowerMode mga::DisplayBuffer::power_mode() const
{
    return power_mode_;
}

mg::NativeDisplayBuffer* mga::DisplayBuffer::native_display_buffer()
{
    return this;
}
