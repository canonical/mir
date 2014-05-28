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
#include "gl_context.h"

#include <boost/throw_exception.hpp>
#include <sstream>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace 
{
class HWC10Context : public mga::SwappingGLContext
{
public:
    HWC10Context(std::function<void()> const& swapping_fn)
        : swapping_fn(std::move(swapping_fn))
    {
    }
    void swap_buffers() const override
    {
        swapping_fn();
    }
private:
    std::function<void()> const swapping_fn;
};
}

mga::HwcFbDevice::HwcFbDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                              std::shared_ptr<HwcWrapper> const& hwc_wrapper,
                              std::shared_ptr<framebuffer_device_t> const& fb_device,
                              std::shared_ptr<HWCVsyncCoordinator> const& coordinator)
    : HWCCommonDevice(hwc_device, coordinator),
      hwc_wrapper(hwc_wrapper), 
      fb_device(fb_device),
      layer_list{{},1}
{
    layer_list.additional_layers_begin()->set_layer_type(mga::LayerType::skip);
}

void mga::HwcFbDevice::gpu_render()
{
    if (auto display_list = layer_list.native_list().lock())
    {
        display_list->dpy = eglGetCurrentDisplay();
        display_list->sur = eglGetCurrentSurface(EGL_DRAW);

        //set() may affect EGL state by calling eglSwapBuffers.
        //HWC 1.0 is the only version of HWC that can do this.
        hwc_wrapper->set(*display_list);
    }
    else
    {
        std::stringstream ss;
        ss << "error locking list during hwc set()";
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mga::HwcFbDevice::prepare()
{
    if (auto display_list = layer_list.native_list().lock())
    {
        hwc_wrapper->prepare(*display_list);
    }
    else
    {
        std::stringstream ss;
        ss << "error accessing list during hwc prepare()";
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mga::HwcFbDevice::render_gl(SwappingGLContext const&)
{
    prepare();
    gpu_render();
}

void mga::HwcFbDevice::prepare_overlays(
    SwappingGLContext const&,
    RenderableList const& list,
    RenderableListCompositor const& compositor)
{
    prepare();
    //hwc 1.0 is weird in that the driver gets to call eglSwapBuffers.
    HWC10Context context(std::bind(&HwcFbDevice::gpu_render, this));
    compositor.render(list, context);
}

void mga::HwcFbDevice::post(mg::Buffer const& buffer)
{
    auto lg = lock_unblanked();

    auto native_buffer = buffer.native_buffer_handle();
    native_buffer->wait_for_content();
    if (fb_device->post(fb_device.get(), native_buffer->handle()) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error posting with fb device"));
    }

    coordinator->wait_for_vsync();
}
