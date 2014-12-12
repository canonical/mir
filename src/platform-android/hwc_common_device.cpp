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
#include "hwc_vsync_coordinator.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <stdexcept>

namespace mga=mir::graphics::android;

namespace
{
static void invalidate_hook(const struct hwc_procs* /*procs*/)
{
}

static void vsync_hook(const struct hwc_procs* procs, int /*disp*/, int64_t /*timestamp*/)
{
    auto self = reinterpret_cast<mga::HWCCallbacks const*>(procs)->self.load();
    if (!self)
        return;
    self->notify_vsync();
}

static void hotplug_hook(const struct hwc_procs* /*procs*/, int /*disp*/, int /*connected*/)
{
}
}

mga::HWCCommonDevice::HWCCommonDevice(std::shared_ptr<HwcWrapper> const& hwc_device,
                                      std::shared_ptr<HWCVsyncCoordinator> const& coordinator)
    : coordinator(coordinator),
      callbacks(std::make_shared<mga::HWCCallbacks>()),
      hwc_device(hwc_device),
      current_mode(mir_power_mode_on)
{
    callbacks->hooks.invalidate = invalidate_hook;
    callbacks->hooks.vsync = vsync_hook;
    callbacks->hooks.hotplug = hotplug_hook;
    callbacks->self = this;

    hwc_device->register_hooks(callbacks);

    try
    {
        turn_screen_on();
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
        std::unique_lock<std::mutex> lg(blanked_mutex);
        try
        {
            turn_screen_off();
        } catch (...)
        {
        }
    }
    callbacks->self = nullptr;
}

void mga::HWCCommonDevice::notify_vsync()
{
    coordinator->notify_vsync();
}

void mga::HWCCommonDevice::mode(MirPowerMode mode_request)
{
    std::unique_lock<std::mutex> lg(blanked_mutex);

    if ((mode_request == mir_power_mode_suspend) ||
        (mode_request == mir_power_mode_standby))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot set to suspend or standby"));
    }

    if ((mode_request == mir_power_mode_on) &&
        (current_mode == mir_power_mode_off))
    {
        turn_screen_on();
    }
    else if ((mode_request == mir_power_mode_off) &&
             (current_mode == mir_power_mode_on))
    {
        turn_screen_off();
    }

    current_mode = mode_request;
    blanked_cond.notify_all();
}

std::unique_lock<std::mutex> mga::HWCCommonDevice::lock_unblanked()
{
    std::unique_lock<std::mutex> lg(blanked_mutex);
    while(current_mode == mir_power_mode_off)
        blanked_cond.wait(lg);
    return std::move(lg);
}

void mga::HWCCommonDevice::turn_screen_on() const
{
    hwc_device->display_on();
    hwc_device->vsync_signal_on();
}

void mga::HWCCommonDevice::turn_screen_off()
{
    hwc_device->vsync_signal_off();
    hwc_device->display_off();
    turned_screen_off();
}

void mga::HWCCommonDevice::turned_screen_off()
{
}

bool mga::HWCCommonDevice::apply_orientation(MirOrientation) const
{
    return false; 
}
