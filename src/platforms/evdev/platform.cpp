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
#include "mir/dispatch/readable_fd.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/module_properties.h"

#include "mir/input/input_device_registry.h"
#include "mir/input/input_device.h"
#include "mir/input/input_report.h"
#include "mir/fd.h"

#define MIR_LOG_COMPONENT "evdev-input"
#include "mir/log.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <libinput.h>

namespace mi = mir::input;
namespace md = mir::dispatch;
namespace mu = mir::udev;
namespace mie = mi::evdev;

mie::Platform::Platform(std::shared_ptr<InputDeviceRegistry> const& registry,
                              std::shared_ptr<InputReport> const& report,
                              std::unique_ptr<udev::Context>&& udev_context,
                              std::unique_ptr<udev::Monitor>&& monitor)
    : report(report), udev_context(std::move(udev_context)), monitor(std::move(monitor)),
      input_device_registry(registry),
      platform_dispatchable{std::make_shared<md::MultiplexingDispatchable>()},
      monitor_dispatchable{
          std::make_shared<md::ReadableFd>(
              Fd{IntOwnedFd{this->monitor->fd()}},
              [this](){process_changes();}
              )},
      lib{make_libinput()},
      libinput_dispatchable{
          std::make_shared<md::ReadableFd>(
              Fd{IntOwnedFd{libinput_get_fd(lib.get())}},
              [this](){process_input_events();}
              )}
{
    this->monitor->filter_by_subsystem("input");
    this->monitor->enable();

}

std::shared_ptr<mir::dispatch::Dispatchable> mie::Platform::dispatchable()
{
    return platform_dispatchable;
}

void mie::Platform::start()
{
    platform_dispatchable->add_watch(monitor_dispatchable);
    platform_dispatchable->add_watch(libinput_dispatchable);
    scan_for_devices();
}

void mie::Platform::process_input_events()
{
    int status = libinput_dispatch(lib.get());
    if (status != 0)
        BOOST_THROW_EXCEPTION(boost::enable_error_info(std::runtime_error("libinput_dispatch_failed"))
                              << boost::errinfo_errno(-status));

    using EventType = std::unique_ptr<libinput_event,void(*)(libinput_event*)>;
    auto next_event = [lilib=lib.get()]
    {
        return EventType(libinput_get_event(lilib), libinput_event_destroy);
    };

    while(auto ev = next_event())
    {
        auto dev = find_device(
            libinput_device_get_device_group(
                libinput_event_get_device(ev.get())
            ));
        if (dev != end(devices))
            (*dev)->process_event(ev.get());
    }
}

void mie::Platform::process_changes()
{
    monitor->process_events(
        [this](mu::Monitor::EventType event, mu::Device const& dev)
        {
            if (!dev.devnode())
                return;
            if (event == mu::Monitor::ADDED)
            {
                log_info("udev:device added %s", dev.devnode());
                device_added(dev);
            }
            if (event == mu::Monitor::REMOVED)
            {
                log_info("udev:device removed %s", dev.devnode());
                device_removed(dev);
            }
            if (event == mu::Monitor::CHANGED)
            {
                log_info("udev:device changed %s", dev.devnode());
                device_changed(dev);
            }
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
        {
            log_info("udev:device added %s", device.devnode());
            device_added(device);
        }
    }
}

void mie::Platform::device_added(mu::Device const& dev)
{
    if (end(devices) != find_device(dev.devnode()))
        return;

    auto device_ptr = make_libinput_device(lib.get(), dev.devnode());

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
        (*device_it)->add_device_of_group(dev.devnode(), move(device_ptr));
        log_debug("Device %s is part of an already opened device group", dev.devnode());
        return;
    }

    try
    {
        devices.emplace_back(std::make_shared<mie::LibInputDevice>(report, dev.devnode(), move(device_ptr)));

        input_device_registry->add_device(devices.back());

        report->opened_input_device(dev.devnode(), "evdev-input");
    } catch(...)
    {
        mir::log_error("Failure opening device %s", dev.devnode());
        report->failed_to_open_input_device(dev.devnode(), "evdev-input");
    }
}

void mie::Platform::device_removed(mu::Device const& dev)
{
    auto known_device_pos = find_device(dev.devnode());

    if (known_device_pos == end(devices))
        return;

    input_device_registry->remove_device(*known_device_pos);
    devices.erase(known_device_pos);
}


auto mie::Platform::find_device(char const* devnode) -> decltype(devices)::iterator
{
    return std::find_if(
        begin(devices),
        end(devices),
        [devnode](auto const& item)
        {
            return item->is_in_group(devnode);
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
            return devgroup == item->group();
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
    platform_dispatchable->remove_watch(monitor_dispatchable);
    platform_dispatchable->remove_watch(libinput_dispatchable);
    while (!devices.empty())
    {
        input_device_registry->remove_device(devices.back());
        devices.pop_back();
    }
}
