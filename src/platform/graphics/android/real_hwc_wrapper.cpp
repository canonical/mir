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

#include "real_hwc_wrapper.h"
#include "hwc_common_device.h"
#include "hwc_logger.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>

namespace mga=mir::graphics::android;

mga::RealHwcWrapper::RealHwcWrapper(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<mga::HwcLogger> const& logger)
    : hwc_device(hwc_device),
      logger(logger)
{
}

void mga::RealHwcWrapper::prepare(hwc_display_contents_1_t& display_list) const
{
    //note, although we only have a primary display right now,
    //      set the external and virtual displays to null as some drivers check for that
    hwc_display_contents_1_t* displays[num_displays] {&display_list, nullptr, nullptr};

    logger->log_list_submitted_to_prepare(display_list);
    if (auto rc = hwc_device->prepare(hwc_device.get(), 1, displays))
    {
        std::stringstream ss;
        ss << "error during hwc prepare(). rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    logger->log_prepare_done(display_list);
}

void mga::RealHwcWrapper::set(hwc_display_contents_1_t& display_list) const
{
    hwc_display_contents_1_t* displays[num_displays] {&display_list, nullptr, nullptr};

    logger->log_set_list(display_list);
    if (auto rc = hwc_device->set(hwc_device.get(), 1, displays))
    {
        std::stringstream ss;
        ss << "error during hwc prepare(). rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mga::RealHwcWrapper::register_hooks(std::shared_ptr<HWCCallbacks> const& callbacks)
{
    hwc_device->registerProcs(hwc_device.get(), reinterpret_cast<hwc_procs_t*>(callbacks.get()));
    registered_callbacks = callbacks;
}

void mga::RealHwcWrapper::vsync_signal_on() const
{
    if (auto rc = hwc_device->eventControl(hwc_device.get(), 0, HWC_EVENT_VSYNC, 1))
    {
        std::stringstream ss;
        ss << "error turning vsync signal on. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    logger->log_vsync_on();
}

void mga::RealHwcWrapper::vsync_signal_off() const
{
    if (auto rc = hwc_device->eventControl(hwc_device.get(), 0, HWC_EVENT_VSYNC, 0))
    {
        std::stringstream ss;
        ss << "error turning vsync signal off. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    logger->log_vsync_off();
}

void mga::RealHwcWrapper::display_on() const
{
    if (auto rc = hwc_device->blank(hwc_device.get(), HWC_DISPLAY_PRIMARY, 0))
    {
        std::stringstream ss;
        ss << "error turning display on. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    logger->log_display_on();
}

void mga::RealHwcWrapper::display_off() const
{
    if (auto rc = hwc_device->blank(hwc_device.get(), HWC_DISPLAY_PRIMARY, 1))
    {
        std::stringstream ss;
        ss << "error turning display off. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    logger->log_display_off();
}
