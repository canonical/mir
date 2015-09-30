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
#include "hwc_report.h"
#include "display_disconnected_exception.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace mga=mir::graphics::android;

namespace
{
int num_displays(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays)
{
    return std::distance(displays.begin(), 
        std::find_if(displays.begin(), displays.end(),
            [](hwc_display_contents_1_t* d){ return d == nullptr; }));
}

mga::DisplayName display_name(int raw_name)
{
    switch(raw_name)
    {
        default:
        case HWC_DISPLAY_PRIMARY:
            return mga::DisplayName::primary;
        case HWC_DISPLAY_EXTERNAL:
            return mga::DisplayName::external;
        case HWC_DISPLAY_VIRTUAL:
            return mga::DisplayName::virt;
    }
}

//note: The destruction ordering of RealHwcWrapper should be enough to ensure that the
//callbacks are not called after the hwc module is closed. However, some badly synchronized
//drivers continue to call the hooks for a short period after we call close(). (LP: 1364637)
static std::mutex callback_lock;
static void invalidate_hook(const struct hwc_procs* procs)
{
    mga::HwcCallbacks const* callbacks{nullptr};
    std::unique_lock<std::mutex> lk(callback_lock);
    if ((callbacks = reinterpret_cast<mga::HwcCallbacks const*>(procs)) && callbacks->self)
        callbacks->self->invalidate();
}

static void vsync_hook(const struct hwc_procs* procs, int display, int64_t timestamp)
{
    mga::HwcCallbacks const* callbacks{nullptr};
    std::unique_lock<std::mutex> lk(callback_lock);
    if ((callbacks = reinterpret_cast<mga::HwcCallbacks const*>(procs)) && callbacks->self)
        callbacks->self->vsync(display_name(display), std::chrono::nanoseconds{timestamp});
}

static void hotplug_hook(const struct hwc_procs* procs, int display, int connected)
{
    mga::HwcCallbacks const* callbacks{nullptr};
    std::unique_lock<std::mutex> lk(callback_lock);
    if ((callbacks = reinterpret_cast<mga::HwcCallbacks const*>(procs)) && callbacks->self)
        callbacks->self->hotplug(display_name(display), connected);
}
static mga::HwcCallbacks hwc_callbacks{{invalidate_hook, vsync_hook, hotplug_hook}, nullptr};

}

mga::RealHwcWrapper::RealHwcWrapper(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<mga::HwcReport> const& report) :
    hwc_device(hwc_device),
    report(report)
{
    std::unique_lock<std::mutex> lk(callback_lock);
    hwc_callbacks.self = this;
    hwc_device->registerProcs(hwc_device.get(), reinterpret_cast<hwc_procs_t*>(&hwc_callbacks));
}

mga::RealHwcWrapper::~RealHwcWrapper()
{
    std::unique_lock<std::mutex> lk(callback_lock);
    hwc_callbacks.self = nullptr;
}

void mga::RealHwcWrapper::prepare(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    report->report_list_submitted_to_prepare(displays);
    if (auto rc = hwc_device->prepare(hwc_device.get(), num_displays(displays),
        const_cast<hwc_display_contents_1**>(displays.data())))
    {
        std::stringstream ss;
        ss << "error during hwc prepare(). rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }

    report->report_prepare_done(displays);
}

void mga::RealHwcWrapper::set(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    report->report_set_list(displays);
    auto const num_displays = ::num_displays(displays);
    if (auto rc = hwc_device->set(hwc_device.get(), num_displays,
        const_cast<hwc_display_contents_1**>(displays.data())))
    {
        if (num_displays > 1 && !display_connected(DisplayName::external))
        {
            BOOST_THROW_EXCEPTION(mga::DisplayDisconnectedException());
        }

        std::stringstream ss;
        ss << "error during hwc set(). rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    report->report_set_done(displays);
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

void mga::RealHwcWrapper::subscribe_to_events(
        void const* subscriber,
        std::function<void(DisplayName, std::chrono::nanoseconds)> const& vsync,
        std::function<void(DisplayName, bool)> const& hotplug,
        std::function<void()> const& invalidate)
{
    std::unique_lock<std::mutex> lk(callback_map_lock);
    callback_map[subscriber] = {vsync, hotplug, invalidate};
}

void mga::RealHwcWrapper::unsubscribe_from_events(void const* subscriber) noexcept
{
    std::unique_lock<std::mutex> lk(callback_map_lock);
    auto it = callback_map.find(subscriber);
    if (it != callback_map.end())
        callback_map.erase(it);
}

void mga::RealHwcWrapper::vsync(DisplayName name, std::chrono::nanoseconds timestamp) noexcept
{
    std::unique_lock<std::mutex> lk(callback_map_lock);
    for(auto const& callbacks : callback_map)
    {
        try
        {
            callbacks.second.vsync(name, timestamp);
        }
        catch (...)
        {
        }
    }
}

void mga::RealHwcWrapper::hotplug(DisplayName name, bool connected) noexcept
{
    std::unique_lock<std::mutex> lk(callback_map_lock);
    for(auto const& callbacks : callback_map)
    {
        try
        {
            callbacks.second.hotplug(name, connected);
        }
        catch (...)
        {
        }
    }
}

void mga::RealHwcWrapper::invalidate() noexcept
{
    std::unique_lock<std::mutex> lk(callback_map_lock);
    for(auto const& callbacks : callback_map)
    {
        try
        {
            callbacks.second.invalidate();
        }
        catch (...)
        {
        }
    }
}

std::vector<mga::ConfigId> mga::RealHwcWrapper::display_configs(DisplayName display_name) const
{
    //No way to get the number of display configs. SF uses 128 possible spots, but that seems excessive.
    static size_t const max_configs = 16;
    size_t num_configs = max_configs;
    static uint32_t display_config[max_configs] = {};
    if (hwc_device->getDisplayConfigs(hwc_device.get(), display_name, display_config, &num_configs))
        return {};

    auto i = 0u;
    std::vector<mga::ConfigId> config_ids{std::min(max_configs, num_configs)};
    for(auto& id : config_ids)
        id = mga::ConfigId{display_config[i++]};
    return config_ids;
}

int mga::RealHwcWrapper::display_attributes(
    DisplayName display_name, ConfigId config, uint32_t const* attributes, int32_t* values) const
{
    return hwc_device->getDisplayAttributes(
        hwc_device.get(), display_name, config.as_value(), attributes, values);
}

void mga::RealHwcWrapper::power_mode(DisplayName display_name, PowerMode mode) const
{
    if (auto rc = hwc_device->setPowerMode(hwc_device.get(), display_name, static_cast<int>(mode)))
    {
        std::stringstream ss;
        ss << "error setting power mode. rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    report->report_power_mode(mode);
}

bool mga::RealHwcWrapper::has_active_config(DisplayName display_name) const
{
    int const no_active_config = -1;
    return hwc_device->getActiveConfig(hwc_device.get(), display_name) != no_active_config;
}

mga::ConfigId mga::RealHwcWrapper::active_config_for(DisplayName display_name) const
{
    int id = hwc_device->getActiveConfig(hwc_device.get(), display_name);
    if (id == -1)
    {
        std::stringstream ss;
        ss << "No active configuration for display: " << display_name;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
    return mga::ConfigId{static_cast<uint32_t>(id)};
}

void mga::RealHwcWrapper::set_active_config(DisplayName display_name, ConfigId id) const
{
    int rc = hwc_device->setActiveConfig(hwc_device.get(), display_name, id.as_value());
    if (rc < 0)
        BOOST_THROW_EXCEPTION(std::system_error(rc, std::system_category(), "unable to set active display config"));
}

bool mga::RealHwcWrapper::display_connected(DisplayName display_name) const
{
    size_t num_configs = 0;
    return hwc_device->getDisplayConfigs(hwc_device.get(), display_name, nullptr, &num_configs) == 0;
}
