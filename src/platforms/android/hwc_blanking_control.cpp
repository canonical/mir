/*
 * Copyright © 2014 Canonical Ltd.
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

#include "hwc_configuration.h"
#include "hwc_wrapper.h"
#include "android_format_conversion-inl.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <chrono>

namespace mga = mir::graphics::android;

namespace
{
MirPixelFormat determine_hwc_fb_format()
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

using namespace std::chrono;
template<class Rep, class Period>
double period_to_hz(duration<Rep,Period> period_duration)
{
    if (period_duration.count() <= 0)
        return 0.0;
    return duration<double>{1} / period_duration;
}
}

mga::HwcBlankingControl::HwcBlankingControl(
    std::shared_ptr<mga::HwcWrapper> const& hwc_device) :
    hwc_device{hwc_device},
    off{false},
    format(determine_hwc_fb_format())
{
}

void mga::HwcBlankingControl::power_mode(DisplayName display_name, MirPowerMode mode_request)
{
    if (mode_request == mir_power_mode_on)
    {
        hwc_device->display_on(display_name);
        hwc_device->vsync_signal_on(display_name);
        off = false;
    }
    //suspend, standby, and off all count as off
    else if (!off)
    {
        hwc_device->vsync_signal_off(display_name);
        hwc_device->display_off(display_name);
        off = true;
    }
}

mga::DisplayAttribs mga::HwcBlankingControl::active_attribs_for(DisplayName display_name)
{
    auto configs = hwc_device->display_configs(display_name);
    if (configs.empty())
    {
        if (display_name == mga::DisplayName::primary)
            BOOST_THROW_EXCEPTION(std::runtime_error("primary display disconnected"));
        else   
            return {{}, {}, 0.0, false, format, quirks.num_framebuffers()};
    }

    /* note: some drivers (qcom msm8960) choke if this is not the same size array
       as the one surfaceflinger submits */
    static uint32_t const attributes[] =
    {
        HWC_DISPLAY_WIDTH,
        HWC_DISPLAY_HEIGHT,
        HWC_DISPLAY_VSYNC_PERIOD,
        HWC_DISPLAY_DPI_X,
        HWC_DISPLAY_DPI_Y,
        HWC_DISPLAY_NO_ATTRIBUTE,
    };

    int32_t values[sizeof(attributes) / sizeof (attributes[0])] = {};
    /* the first config is the active one in hwc 1.1 to hwc 1.3. */
    hwc_device->display_attributes(display_name, configs.front(), attributes, values);
    return {
        {values[0], values[1]},
        {0, 0}, //TODO: convert DPI to MM and return
        period_to_hz(std::chrono::nanoseconds{values[2]}),
        true,
        format,
        quirks.num_framebuffers()
    };
}

mga::ConfigChangeSubscription mga::HwcBlankingControl::subscribe_to_config_changes(
    std::function<void()> const&)
{
    return nullptr;
}
