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
#include "hwc_report.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace mga=mir::graphics::android;

mga::RealHwcWrapper::RealHwcWrapper(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<mga::HwcReport> const& report)
    : hwc_device(hwc_device),
      report(report)
{
}

namespace
{
int num_displays(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays)
{
    return std::distance(displays.begin(), 
        std::find_if(displays.begin(), displays.end(),
            [](hwc_display_contents_1_t* d){ return d == nullptr; }));
}
}

void mga::RealHwcWrapper::prepare(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    //TODO: convert report to reporting multimonitor
    if (displays[0]) report->report_list_submitted_to_prepare(*displays[0]);
    if (auto rc = hwc_device->prepare(hwc_device.get(), num_displays(displays),
        const_cast<hwc_display_contents_1**>(displays.data())))
    {
        std::stringstream ss;
        ss << "error during hwc prepare(). rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }

    if (displays[0]) report->report_prepare_done(*displays[0]);
}

void mga::RealHwcWrapper::set(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    if (displays[0]) report->report_set_list(*displays[0]);
    if (auto rc = hwc_device->set(hwc_device.get(), num_displays(displays),
        const_cast<hwc_display_contents_1**>(displays.data())))
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

void mga::RealHwcWrapper::vsync_signal_on(DisplayName display_name) const
{
    if (auto rc = hwc_device->eventControl(hwc_device.get(), display_name, HWC_EVENT_VSYNC, 1))
    {
        std::stringstream ss;
        ss << "error turning vsync signal on. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    report->report_vsync_on();
}

void mga::RealHwcWrapper::vsync_signal_off(DisplayName display_name) const
{
    if (auto rc = hwc_device->eventControl(hwc_device.get(), display_name, HWC_EVENT_VSYNC, 0))
    {
        std::stringstream ss;
        ss << "error turning vsync signal off. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    report->report_vsync_off();
}

void mga::RealHwcWrapper::display_on(DisplayName display_name) const
{
    if (auto rc = hwc_device->blank(hwc_device.get(), display_name, 0))
    {
        std::stringstream ss;
        ss << "error turning display on. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    report->report_display_on();
}

void mga::RealHwcWrapper::display_off(DisplayName display_name) const
{
    if (auto rc = hwc_device->blank(hwc_device.get(), display_name, 1))
    {
        std::stringstream ss;
        ss << "error turning display off. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    report->report_display_off();
}
