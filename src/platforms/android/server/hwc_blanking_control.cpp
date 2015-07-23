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
#include "mir/raii.h"
#include "mir/graphics/android/android_format_conversion-inl.h"
#include "mir/geometry/length.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <system_error>
#include <chrono>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

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

namespace
{
int dpi_to_mm(uint32_t dpi, int pixel_num)
{
    if (dpi == 0) return 0;
    float dpi_inches = dpi / 1000.0f; //android multiplies by 1000
    geom::Length length(pixel_num / dpi_inches, geom::Length::Units::inches);
    return length.as(geom::Length::Units::millimetres);
}

mg::DisplayConfigurationOutput populate_config(
    mga::DisplayName name,
    geom::Size pixel_size,
    double vrefresh_hz,
    geom::Size mm_size,
    MirPowerMode external_mode,
    MirPixelFormat display_format,
    bool connected)
{
    geom::Point const origin{0,0};
    size_t const preferred_format_index{0};
    size_t const preferred_mode_index{0};
    std::vector<mg::DisplayConfigurationMode> external_modes;
    if (connected)
        external_modes.emplace_back(mg::DisplayConfigurationMode{pixel_size, vrefresh_hz});

    auto type = mg::DisplayConfigurationOutputType::lvds;
    if (name == mga::DisplayName::external)
        type = mg::DisplayConfigurationOutputType::displayport;
    
    return {
        static_cast<mg::DisplayConfigurationOutputId>(name),
        mg::DisplayConfigurationCardId{0},
        type,
        {display_format},
        external_modes,
        preferred_mode_index,
        mm_size,
        connected,
        connected,
        origin,
        preferred_format_index,
        display_format,
        external_mode,
        mir_orientation_normal
    };
}

mg::DisplayConfigurationOutput display_config_for(
    mga::DisplayName display_name,
    mga::ConfigId id,
    MirPixelFormat format,
    std::shared_ptr<mga::HwcWrapper> const& hwc_device
)
{
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

    auto rc = hwc_device->display_attributes(display_name, id, attributes, values);

    if (rc < 0)
    {
        if (display_name == mga::DisplayName::primary)
            BOOST_THROW_EXCEPTION(std::system_error(rc, std::system_category(), "primary display disconnected"));
        else
            return populate_config(display_name, {0,0}, 0.0f, {0,0}, mir_power_mode_off, mir_pixel_format_invalid, false);
    }

    return populate_config(
        display_name,
        {values[0], values[1]},
        period_to_hz(std::chrono::nanoseconds{values[2]}),
        {dpi_to_mm(values[3], values[0]), dpi_to_mm(values[4], values[1])},
        mir_power_mode_off,
        format,
        true);
}

mga::ConfigChangeSubscription subscribe_to_config_changes(
    std::shared_ptr<mga::HwcWrapper> const& hwc_device,
    void const* subscriber,
    std::function<void()> const& hotplug,
    std::function<void(mga::DisplayName)> const& vsync)
{
    return std::make_shared<
        mir::raii::PairedCalls<std::function<void()>, std::function<void()>>>(
        [hotplug, vsync, subscriber, hwc_device]{
            hwc_device->subscribe_to_events(subscriber,
                [vsync](mga::DisplayName name, std::chrono::nanoseconds){ vsync(name); },
                [hotplug](mga::DisplayName, bool){ hotplug(); },
                []{});
        },
        [subscriber, hwc_device]{
            hwc_device->unsubscribe_from_events(subscriber);
        });
}
}

mg::DisplayConfigurationOutput mga::HwcBlankingControl::active_config_for(DisplayName display_name)
{
    auto configs = hwc_device->display_configs(display_name);
    if (configs.empty())
    {
        if (display_name == mga::DisplayName::primary)
            BOOST_THROW_EXCEPTION(std::runtime_error("primary display disconnected"));
        else   
            return populate_config(display_name, {0,0}, 0.0f, {0,0}, mir_power_mode_off, mir_pixel_format_invalid, false);
    }

    return display_config_for(display_name, configs.front(), format, hwc_device);
}

mga::ConfigChangeSubscription mga::HwcBlankingControl::subscribe_to_config_changes(
    std::function<void()> const& hotplug,
    std::function<void(DisplayName)> const& vsync)
{
    return ::subscribe_to_config_changes(hwc_device, this, hotplug, vsync);
}

mga::HwcPowerModeControl::HwcPowerModeControl(
    std::shared_ptr<mga::HwcWrapper> const& hwc_device) :
    hwc_device{hwc_device},
    format(determine_hwc_fb_format())
{
}

void mga::HwcPowerModeControl::power_mode(DisplayName display_name, MirPowerMode mode_request)
{
    PowerMode mode;
    switch (mode_request)
    {
        case mir_power_mode_on:
            mode = PowerMode::normal;
            break;
        case mir_power_mode_standby:
            mode = PowerMode::doze;
            break;
        case mir_power_mode_suspend:
            mode = PowerMode::doze_suspend;
            break;
        case mir_power_mode_off:
            mode = PowerMode::off;
            break;
        default:
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid power mode"));
    }
    hwc_device->power_mode(display_name, mode);
}

mg::DisplayConfigurationOutput mga::HwcPowerModeControl::active_config_for(DisplayName display_name)
{
    auto configs = hwc_device->display_configs(display_name);
    if (configs.empty())
    {
        if (display_name == mga::DisplayName::primary)
            BOOST_THROW_EXCEPTION(std::runtime_error("primary display disconnected"));
        else
            return populate_config(display_name, {0,0}, 0.0f, {0,0}, mir_power_mode_off, mir_pixel_format_invalid, false);
    }

    /* the first config is the active one in hwc 1.1 to hwc 1.3. */
    ConfigId active_config_id = configs.front();
    if (hwc_device->has_active_config(display_name))
    {
        active_config_id = hwc_device->active_config_for(display_name);
    }
    else
    {
        //If no active config, just choose the first from the list
        hwc_device->set_active_config(display_name, configs.front());
    }

    return display_config_for(display_name, active_config_id, format, hwc_device);
}

mga::ConfigChangeSubscription mga::HwcPowerModeControl::subscribe_to_config_changes(
    std::function<void()> const& hotplug,
    std::function<void(DisplayName)> const& vsync)
{
    return ::subscribe_to_config_changes(hwc_device, this, hotplug, vsync);
}
