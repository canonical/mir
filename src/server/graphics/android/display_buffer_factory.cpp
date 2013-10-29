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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_buffer_factory.h"
#include "display_resource_factory.h"
#include "display_buffer.h"
#include "display_device.h"

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/egl_resources.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace
{

static EGLint const dummy_pbuffer_attribs[] =
{
    EGL_WIDTH, 1,
    EGL_HEIGHT, 1,
    EGL_NONE
};

static EGLint const default_egl_context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static EGLint const default_egl_config_attr [] =
{
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

static EGLConfig select_egl_config(EGLDisplay)
{
    return reinterpret_cast<EGLConfig>(0x2);
}

static EGLConfig select_egl_config_with_visual_id(EGLDisplay egl_display, int required_visual_id)
{
    int num_potential_configs;
    EGLint num_match_configs;

    eglGetConfigs(egl_display, NULL, 0, &num_potential_configs);
    std::vector<EGLConfig> config_slots(num_potential_configs);

    /* upon return, this will fill config_slots[0:num_match_configs] with the matching */
    eglChooseConfig(egl_display, default_egl_config_attr,
                    config_slots.data(), num_potential_configs, &num_match_configs);
    config_slots.resize(num_match_configs);

    /* why check manually for EGL_NATIVE_VISUAL_ID instead of using eglChooseConfig? the egl
     * specification does not list EGL_NATIVE_VISUAL_ID as something it will check for in
     * eglChooseConfig */
    auto const pegl_config = std::find_if(begin(config_slots), end(config_slots),
        [&](EGLConfig& current) -> bool
        {
            int visual_id;
            eglGetConfigAttrib(egl_display, current, EGL_NATIVE_VISUAL_ID, &visual_id);
            return (visual_id == required_visual_id);
        });

    if (pegl_config == end(config_slots))
        BOOST_THROW_EXCEPTION(std::runtime_error("could not select EGL config for use with framebuffer"));

    return *pegl_config;
}

EGLDisplay create_and_initialize_display()
{
    EGLint major, minor;

    auto egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("eglGetDisplay failed\n"));

    if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("eglInitialize failure\n"));

    if ((major != 1) || (minor != 4))
        BOOST_THROW_EXCEPTION(std::runtime_error("must have EGL 1.4\n"));
    return egl_display;
}

}

mga::DisplayBufferFactory::DisplayBufferFactory(
    std::shared_ptr<mga::DisplayResourceFactory> const& res_factory,
    std::shared_ptr<mg::DisplayReport> const& display_report)
    : res_factory(res_factory),
      display_report(display_report),
      display(create_and_initialize_display()),
      force_backup_display(false)
{
    try
    {
        hwc_native = res_factory->create_hwc_native_device();
    } catch (...)
    {
        force_backup_display = true;
    }

    //HWC 1.2 not supported yet. make an attempt to use backup display
    if (hwc_native && hwc_native->common.version == HWC_DEVICE_API_VERSION_1_2)
    {
        force_backup_display = true;
    }

    if (!force_backup_display && hwc_native->common.version == HWC_DEVICE_API_VERSION_1_1)
    {
        config = select_egl_config(display); 
    }
    else
    {
        fb_native = res_factory->create_fb_native_device();
        config = select_egl_config_with_visual_id(display, fb_native->format);
    }
}

EGLConfig mga::DisplayBufferFactory::egl_config()
{
    return config;
}

EGLDisplay mga::DisplayBufferFactory::egl_display()
{
    return display;
}

EGLDisplay mga::DisplayBufferFactory::shared_egl_context()
{
    return shared_context;
}

std::shared_ptr<mga::DisplayDevice> mga::DisplayBufferFactory::create_display_device()
{
    std::shared_ptr<mga::DisplayDevice> device; 
    if (force_backup_display)
    {
        device = res_factory->create_fb_device(fb_native);
        display_report->report_gpu_composition_in_use();
    }
    else
    {
        if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_1)
        {
            device = res_factory->create_hwc11_device(hwc_native);
            display_report->report_hwc_composition_in_use(1,1);
        }
        if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
        {
            device = res_factory->create_hwc10_device(hwc_native, fb_native);
            display_report->report_hwc_composition_in_use(1,0);
        }
    }

    return device;
}

std::unique_ptr<mg::DisplayBuffer> mga::DisplayBufferFactory::create_display_buffer(
    std::shared_ptr<DisplayDevice> const& display_device)
{
    auto native_window = res_factory->create_native_window(display_device);
    return std::unique_ptr<mg::DisplayBuffer>(
        new DisplayBuffer(display_device, native_window, display, config, shared_context)); 
}
