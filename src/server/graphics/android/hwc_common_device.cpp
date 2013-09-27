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
#include "hwc_vsync_coordinator.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
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

mga::HWCCommonDevice::HWCCommonDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                                      std::shared_ptr<mga::HWCVsyncCoordinator> const& coordinator)
    : hwc_device(hwc_device),
      coordinator(coordinator),
      blanked(false)
{
    callbacks.hooks.invalidate = invalidate_hook;
    callbacks.hooks.vsync = vsync_hook;
    callbacks.hooks.hotplug = hotplug_hook;
    callbacks.self = this;

    hwc_device->registerProcs(hwc_device.get(), &callbacks.hooks);

    int err = hwc_device->blank(hwc_device.get(), HWC_DISPLAY_PRIMARY, 0);
    if (err)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Could not unblank display")) <<
            boost::errinfo_errno(-err));
    }
    
    err = hwc_device->eventControl(hwc_device.get(), 0, HWC_EVENT_VSYNC, 1);
    if (err)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Could not enable HWC vsync "
                                   "notifications")) <<
            boost::errinfo_errno(-err));
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

void mga::HWCCommonDevice::notify_vsync()
{
    coordinator->notify_vsync();
}

void mga::HWCCommonDevice::blank_or_unblank_screen(bool blank_request)
{
    std::unique_lock<std::mutex> lg(blanked_mutex);

    if (blank_request != blanked)
    {
        if(auto err = hwc_device->blank(hwc_device.get(), HWC_DISPLAY_PRIMARY, blank_request))
        {
            std::string blanking_status_msg = "Could not " + 
                (blank_request ? std::string("blank") : std::string("unblank")) + " display";
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(blanking_status_msg)) <<
                boost::errinfo_errno(-err));
        }    
        blanked = blank_request;
        blanked_cond.notify_all();
    }
}

std::unique_lock<std::mutex> mga::HWCCommonDevice::lock_unblanked()
{
    std::unique_lock<std::mutex> lg(blanked_mutex);
    while(blanked)
        blanked_cond.wait(lg);
    return std::move(lg);
}
