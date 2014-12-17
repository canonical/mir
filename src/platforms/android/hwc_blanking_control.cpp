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
