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

namespace mga = mir::graphics::android;

mga::HwcBlankingControl::HwcBlankingControl(
    std::shared_ptr<mga::HwcWrapper> const& hwc_device) :
    hwc_device{hwc_device}
{
}

void mga::HwcBlankingControl::power_mode(DisplayName display_name, MirPowerMode mode_request)
{
    if (mode_request == mir_power_mode_on)
    {
        hwc_device->display_on(display_name);
        hwc_device->vsync_signal_on(display_name);
    }
    else
    {
        hwc_device->vsync_signal_off(display_name);
        hwc_device->display_off(display_name);
    }
}

mga::DisplayAttribs mga::HwcBlankingControl::display_attribs(DisplayName)
{
    size_t num_configs = 1;
    uint32_t display_config = 0u;
    if (hwc_device->getDisplayConfigs(hwc_device.get(), name, &display_config, &num_configs))
        BOOST_THROW_EXCEPTION(std::runtime_error("could not determine hwc display config"));

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

    int32_t size_values[sizeof(display_attribute_request) / sizeof (display_attribute_request[0])] = {};
    hwc_device->getDisplayAttributes(hwc_device.get(), name, display_config,
                                     display_attribute_request, size_values);

    mga::HwcAttribs attribs
    {
        {size_values[0], size_values[1]},
        {size_values[3], size_values[4]},
        (size_values[2] > 0 ) ? 1000000000.0/size_values[2] : 0.0
    };
    return attribs;
}
//        BOOST_THROW_EXCEPTION(std::runtime_error("could not determine hwc display config"));

#if 0
mga::HwcAttribs mga::RealHwcWrapper::display_attribs(DisplayName name) const
{
}
#endif

