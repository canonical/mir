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

#include "hwc_common_device.h"
#include "hwc_wrapper.h"
#include "hwc_configuration.h"

namespace mga=mir::graphics::android;

mga::HWCCommonDevice::HWCCommonDevice(
    std::shared_ptr<HwcWrapper> const& hwc_device,
    std::shared_ptr<HwcConfiguration> const& hwc_config) :
    hwc_device(hwc_device),
    hwc_config(hwc_config),
    current_mode(mir_power_mode_on)
{
    try
    {
        mode(mir_power_mode_on);
    } catch (...)
    {
        //TODO: log failure here. some drivers will throw if the screen is already on, which
        //      is not a fatal condition
    }
}

mga::HWCCommonDevice::~HWCCommonDevice() noexcept
{
    if (current_mode == mir_power_mode_on)
    {
        try
        {
            mode(mir_power_mode_off);
        } catch (...)
        {
        }
    }
}

void mga::HWCCommonDevice::mode(MirPowerMode mode_request)
{
    hwc_config->power_mode(mga::DisplayName::primary, mode_request);

    if (mode_request == mir_power_mode_off)
        turned_screen_off();

    //TODO the mode should be in the display mode structure that gets passed to the rest of the system
    current_mode = mode_request;
}

void mga::HWCCommonDevice::turned_screen_off()
{
}
