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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc_fb_device.h"
#include "hwc_vsync_coordinator.h"
#include "framebuffer_bundle.h"
#include "android_format_conversion-inl.h"
#include "hwc_wrapper.h"
#include "hwc_fallback_gl_renderer.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/android/native_buffer.h"
#include "swapping_gl_context.h"

#include <boost/throw_exception.hpp>
#include <sstream>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

mga::HwcFbDevice::HwcFbDevice(
    std::shared_ptr<HwcWrapper> const& hwc_wrapper,
    std::shared_ptr<framebuffer_device_t> const& fb_device,
    std::shared_ptr<HwcConfiguration> const& config,
    std::shared_ptr<HWCVsyncCoordinator> const& coordinator) :
    HWCCommonDevice(hwc_wrapper, config, coordinator),
    hwc_wrapper(hwc_wrapper), 
    fb_device(fb_device),
    layer_list{std::make_shared<IntegerSourceCrop>(), {}, 1}
{
}

void mga::HwcFbDevice::post_gl(SwappingGLContext const& context)
{
    auto& buffer = *context.last_rendered_buffer();
    layer_list.begin()->layer.setup_layer(mga::LayerType::skip, {{0,0},buffer.size()}, false, buffer);

    if (auto display_list = layer_list.native_list().lock())
    {
        hwc_wrapper->prepare(*display_list);
        display_list->dpy = eglGetCurrentDisplay();
        display_list->sur = eglGetCurrentSurface(EGL_DRAW);

        //set() may affect EGL state by calling eglSwapBuffers.
        //HWC 1.0 is the only version of HWC that can do this.
        hwc_wrapper->set(*display_list);
    }
    else
    {
        std::stringstream ss;
        ss << "error accessing list during hwc prepare()";
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }

    auto lg = lock_unblanked();

    buffer = *context.last_rendered_buffer();
    auto native_buffer = buffer.native_buffer_handle();
    native_buffer->ensure_available_for(mga::BufferAccess::read);
    if (fb_device->post(fb_device.get(), native_buffer->handle()) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error posting with fb device"));
    }

    coordinator->wait_for_vsync();
}

bool mga::HwcFbDevice::post_overlays(
    SwappingGLContext const&,
    RenderableList const&,
    RenderableListCompositor const&)
{
    return false;
}
