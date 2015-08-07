/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "platform.h"
#include "libinput_device.h"
#include "libinput_ptr.h"
#include "mir/udev/wrapper.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/module_properties.h"

#include "mir/input/input_device_registry.h"
#include "mir/input/input_device.h"
#include "mir/input/input_report.h"

#define MIR_LOG_COMPONENT "evdev-input"
#include "mir/log.h"

#include <libinput.h>

namespace mi = mir::input;
namespace md = mir::dispatch;
namespace mu = mir::udev;
namespace mie = mi::evdev;

struct mie::MonitorDispatchable : md::Dispatchable
{
    MonitorDispatchable(mie::Platform& platform, int udev_fd)
        : platform(platform), fd{mir::Fd{mir::IntOwnedFd{udev_fd}}}
    {
    }

    Fd watch_fd() const override
    {
        return fd;
    }

    bool dispatch(md::FdEvents events) override
    {
        if (events & md::FdEvent::error)
            return false;

        platform.process_changes();
        return true;
    }

    md::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }

    mie::Platform& platform;
    mir::Fd fd;
};

mie::Platform::DeviceInfo::DeviceInfo(char const* device_path,
                                      LibInputDevicePtr ptr,
                                      std::shared_ptr<LibInputDevice> const& device)
    : first_device{std::move(ptr)}, group{libinput_device_get_device_group(first_device.get())}, device(device)
{
    open_nodes.emplace_back(device_path);
}

void mie::Platform::DeviceInfo::add_to_group(char const* path)
{
    open_nodes.emplace_back(path);
    device->open_device_of_group(path);
}

mie::Platform::Platform(std::shared_ptr<InputDeviceRegistry> const& registry,
                              std::shared_ptr<InputReport> const& report,
                              std::unique_ptr<udev::Context>&& udev_context,
                              std::unique_ptr<udev::Monitor>&& monitor)
    : report(report), udev_context(std::move(udev_context)), monitor(std::move(monitor)),
      input_device_registry(registry),
      monitor_dispatchable(std::make_shared<MonitorDispatchable>(*this, this->monitor->fd())), lib{make_libinput()}
{
    this->monitor->filter_by_subsystem("input");
    this->monitor->enable();
}

std::shared_ptr<mir::dispatch::Dispatchable> mie::Platform::dispatchable()
{
    return monitor_dispatchable;
}

void mie::Platform::start()
{
    scan_for_devices();
}

void mie::Platform::process_changes()
{
    monitor->process_events(
        [this](mu::Monitor::EventType event, mu::Device const& dev)
        {
            if (!dev.devnode())
                return;
            if (event == mu::Monitor::ADDED)
                device_added(dev);
            if (event == mu::Monitor::REMOVED)
                device_removed(dev);
            if (event == mu::Monitor::CHANGED)
                device_changed(dev);
        });
}

void mie::Platform::scan_for_devices()
{
    mu::Enumerator input_enumerator{udev_context};
    input_enumerator.match_subsystem("input");
    input_enumerator.scan_devices();

    for (auto& device : input_enumerator)
    {
        if (device.devnode() != nullptr)
            device_added(device);
    }
}

void mie::Platform::device_added(mu::Device const& dev)
{
    if (end(devices) != find_device(dev.devnode()))
        return;

    auto device_ptr = make_libinput_device(lib, dev.devnode());

    // libinput might refuse to open certain devices nodes like /dev/input/mice
    // or ignore devices with odd evdev bits/capabilities set
    if (!device_ptr)
    {
        report->failed_to_open_input_device(dev.devnode(), "evdev-input");
        log_info("libinput refused to open device %s", dev.devnode());
        return;
    }

    auto device_it = find_device(libinput_device_get_device_group(device_ptr.get()));
    if (end(devices) != device_it)
    {
        device_it->add_to_group(dev.devnode());
        mir::log_debug("Device %s is part of an already opened device group", dev.devnode());
        return;
    }

    try
    {
        devices.emplace_back(dev.devnode(), std::move(device_ptr), create_device(dev));

        input_device_registry->add_device(devices.back().device);

        mir::log_info("Input device %s opened", dev.devnode());
        report->opened_input_device(dev.devnode(), "evdev-input");
    } catch(...)
    {
        mir::log_error("Failure opening device %s", dev.devnode());
        report->failed_to_open_input_device(dev.devnode(), "evdev-input");
    }
}

std::shared_ptr<mie::LibInputDevice> mie::Platform::create_device(udev::Device const& device) const
{
    return std::make_shared<mie::LibInputDevice>(report, mie::make_libinput(), device.devnode());
}

void mie::Platform::device_removed(mu::Device const& dev)
{
    auto known_device_pos = find_device(dev.devnode());

    if (known_device_pos == end(devices))
        return;

    mir::log_info("Input device %s removed", dev.devnode());
    input_device_registry->remove_device(known_device_pos->device);
    devices.erase(known_device_pos);
}


auto mie::Platform::find_device(char const* devnode) -> decltype(devices)::iterator
{
    return std::find_if(
        begin(devices),
        end(devices),
        [devnode](auto const& item)
        {
            return end(item.open_nodes) != find(begin(item.open_nodes), end(item.open_nodes), std::string{devnode});
        }
        );
}

auto mie::Platform::find_device(libinput_device_group const* devgroup) -> decltype(devices)::iterator
{
    if (devgroup == nullptr)
        return end(devices);
    return std::find_if(
        begin(devices),
        end(devices),
        [devgroup](auto const& item)
        {
            return devgroup == item.group;
        }
        );
}

void mie::Platform::device_changed(mu::Device const& dev)
{
    device_removed(dev);
    device_added(dev);
}

void mie::Platform::stop()
{
    while (!devices.empty())
    {
        input_device_registry->remove_device(devices.back().device);
        devices.pop_back();
    }
}
