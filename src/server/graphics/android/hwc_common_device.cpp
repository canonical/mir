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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc_common_device.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
static void invalidate_hook(const struct hwc_procs* /*procs*/)
{
}

static void vsync_hook(const struct hwc_procs* procs, int /*disp*/, int64_t /*timestamp*/)
{
    auto self = reinterpret_cast<mga::HWCCallbacks const*>(procs)->self;
    self->notify_vsync();
}

static void hotplug_hook(const struct hwc_procs* /*procs*/, int /*disp*/, int /*connected*/)
{
}
}

mga::HWCCommonDevice::HWCCommonDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device)
    : hwc_device(hwc_device), 
      vsync_occurred(false)
{
    callbacks.hooks.invalidate = invalidate_hook;
    callbacks.hooks.vsync = vsync_hook;
    callbacks.hooks.hotplug = hotplug_hook;
    callbacks.self = this;

    hwc_device->registerProcs(hwc_device.get(), &callbacks.hooks);

    if (hwc_device->blank(hwc_device.get(), HWC_DISPLAY_PRIMARY, 0) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("could not blank display"));
    }
    
    if (hwc_device->eventControl(hwc_device.get(), 0, HWC_EVENT_VSYNC, 1) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("could not enable hwc vsync notifications"));
    }
}

mga::HWCCommonDevice::~HWCCommonDevice() noexcept
{
    hwc_device->eventControl(hwc_device.get(), 0, HWC_EVENT_VSYNC, 0);
    hwc_device->blank(hwc_device.get(), HWC_DISPLAY_PRIMARY, 1);
}

geom::PixelFormat mga::HWCCommonDevice::display_format() const
{
    return geom::PixelFormat::abgr_8888;
}

unsigned int mga::HWCCommonDevice::number_of_framebuffers_available() const
{
    return 2u;
}

void mga::HWCCommonDevice::wait_for_vsync()
{
    std::unique_lock<std::mutex> lk(vsync_wait_mutex);
    vsync_occurred = false;
    while(!vsync_occurred)
    {
        vsync_trigger.wait(lk);
    }
}

void mga::HWCCommonDevice::notify_vsync()
{
    std::unique_lock<std::mutex> lk(vsync_wait_mutex);
    vsync_occurred = true;
    vsync_trigger.notify_all();
}
