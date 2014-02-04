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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "framebuffers.h"
#include "android_format_conversion-inl.h"
#include "graphic_buffer_allocator.h"

#include <algorithm>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
MirPixelFormat determine_hwc11_fb_format()
{
    static EGLint const fb_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_FRAMEBUFFER_TARGET_ANDROID, EGL_TRUE,
        EGL_NONE
    };

    EGLConfig fb_egl_config;
    int matching_configs;
    EGLint major, minor;
    auto egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl_display, &major, &minor);
    eglChooseConfig(egl_display, fb_egl_config_attr, &fb_egl_config, 1, &matching_configs);

    MirPixelFormat fb_format;
    if (matching_configs)
    {
        int visual_id;
        eglGetConfigAttrib(egl_display, fb_egl_config, EGL_NATIVE_VISUAL_ID, &visual_id);
        fb_format = mga::to_mir_format(visual_id);
    }
    else
    {
        //we couldn't figure out the fb format via egl. In this case, we
        //assume abgr_8888. HWC api really should provide this information directly.
        fb_format = mir_pixel_format_abgr_8888;
    }

    eglTerminate(egl_display);
    return fb_format;
}

geom::Size determine_hwc11_size(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device)
{
    size_t num_configs = 1;
    uint32_t primary_display_config;
    auto rc = hwc_device->getDisplayConfigs(hwc_device.get(), HWC_DISPLAY_PRIMARY, &primary_display_config, &num_configs);
    if (rc != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("could not determine hwc display config"));
    }

    /* note: some drivers (qcom msm8960) choke if this is not the same size array
       as the one surfaceflinger submits */
    static uint32_t const display_attribute_request[] =
    {
        HWC_DISPLAY_WIDTH,
        HWC_DISPLAY_HEIGHT,
        HWC_DISPLAY_VSYNC_PERIOD,
        HWC_DISPLAY_DPI_X,
        HWC_DISPLAY_DPI_Y,
        HWC_DISPLAY_NO_ATTRIBUTE,
    };

    int32_t size_values[sizeof(display_attribute_request) / sizeof (display_attribute_request[0])];
    hwc_device->getDisplayAttributes(hwc_device.get(), HWC_DISPLAY_PRIMARY, primary_display_config,
                                     display_attribute_request, size_values);

    return {size_values[0], size_values[1]};
}
}

mga::Framebuffers::Framebuffers(
    std::shared_ptr<mga::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<hwc_composer_device_1> const& hwc)
    : format(determine_hwc11_fb_format()),
      size(determine_hwc11_size(hwc))
{
    for(auto i = 0u; i < 2; i++)
    {
        queue.push(buffer_allocator->alloc_buffer_platform(size, format, mga::BufferUsage::use_framebuffer_gles));
    }
}

mga::Framebuffers::Framebuffers(
    std::shared_ptr<mga::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<framebuffer_device_t> const& fb)
    : format{mga::to_mir_format(fb->format)},
      size({fb->width, fb->height})
{
    //guarantee always 2 fb's allocated
    auto fb_num = static_cast<unsigned int>(fb->numFramebuffers);
    fb_num = std::max(2u, fb_num);
    for(auto i = 0u; i < fb_num; i++)
    {
        queue.push(buffer_allocator->alloc_buffer_platform(size, format, mga::BufferUsage::use_framebuffer_gles));
    }
}

MirPixelFormat mga::Framebuffers::fb_format()
{
    return format;
}
geom::Size mga::Framebuffers::fb_size()
{
    return size;
}

std::shared_ptr<mg::Buffer> mga::Framebuffers::buffer_for_render()
{
    std::unique_lock<std::mutex> lk(queue_lock);
    while (buffer_being_rendered)
    {
        cv.wait(lk);
    }

    buffer_being_rendered = queue.front();
    queue.pop();
    return std::shared_ptr<mg::Buffer>(buffer_being_rendered.get(),
        [this](mg::Buffer*)
        {
            std::unique_lock<std::mutex> lk(queue_lock);
            queue.push(buffer_being_rendered);
            buffer_being_rendered.reset();
            cv.notify_all();
        });
}

std::shared_ptr<mg::Buffer> mga::Framebuffers::last_rendered_buffer()
{
    std::unique_lock<std::mutex> lk(queue_lock);
    return queue.back();
}

void mga::Framebuffers::wait_for_consumed_buffer(bool)
{
    //TODO: change swapping so buffer_for_render() does not wait
}
