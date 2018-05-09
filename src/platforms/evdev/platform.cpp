/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include "fd_store.h"

#include "mir/udev/wrapper.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/module_properties.h"
#include "mir/assert_module_entry_point.h"
#include "mir/console_services.h"

#include "mir/input/input_device_registry.h"
#include "mir/input/input_device.h"
#include "mir/input/input_report.h"
#include "mir/fd.h"
#include "mir/raii.h"

#define MIR_LOG_COMPONENT "evdev-input"
#include "mir/log.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <libinput.h>
#include <string>

namespace mi = mir::input;
namespace md = mir::dispatch;
namespace mu = mir::udev;
namespace mie = mi::evdev;

namespace
{

std::string describe(libinput_device* dev)
{
    auto const udev_dev = mir::raii::deleter_for(libinput_device_get_udev_device(dev), &udev_device_unref);
    std::string desc(udev_device_get_devnode(udev_dev.get()));

    char const * const model = udev_device_get_property_value(udev_dev.get(), "ID_MODEL");
    if (model)
        desc += ": " + std::string(model);

    // Yes, we could use std::replace but this will compile smaller and faster
    for (auto &c : desc)
        if (c == '_') c = ' ';

    return desc;
}

class DispatchableUDevMonitor : public md::Dispatchable
{
public:
    DispatchableUDevMonitor(
        mu::Context& context,
        std::function<void(mu::Monitor::EventType, mu::Device const&)> on_event)
        : monitor(context),
          fd{monitor.fd()},
          on_event{std::move(on_event)}
    {
        monitor.filter_by_subsystem("input");
        monitor.enable();

        auto fake_shared_context = std::shared_ptr<mu::Context>{&context, [](auto){}};
        mu::Enumerator device_enumerator{fake_shared_context};

        device_enumerator.match_subsystem("input");
        device_enumerator.scan_devices();

        for (auto const& device : device_enumerator)
        {
            if (device.initialised())
            {
                this->on_event(mu::Monitor::EventType::ADDED, device);
            }
        }
    }

    mir::Fd watch_fd() const override
    {
        return fd;
    }

    bool dispatch(md::FdEvents events) override
    {
        if (events & md::FdEvent::error)
        {
            return false;
        }

        monitor.process_events(on_event);
        return true;
    }

    md::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }

private:
    mu::Monitor monitor;
    mir::Fd fd;
    std::function<void(mu::Monitor::EventType, mu::Device const&)> const on_event;
};

} // namespace

mie::Platform::Platform(
        std::shared_ptr<InputDeviceRegistry> const& registry,
        std::shared_ptr<InputReport> const& report,
        std::unique_ptr<udev::Context>&& udev_context,
        std::shared_ptr<ConsoleServices> const& console) :
    report(report),
    udev_context(std::move(udev_context)),
    input_device_registry(registry),
    console{console},
    platform_dispatchable{std::make_shared<md::MultiplexingDispatchable>()}
{
}

std::shared_ptr<mir::dispatch::Dispatchable> mie::Platform::dispatchable()
{
    return platform_dispatchable;
}

void mie::Platform::start()
{
    lib = make_libinput(&device_fds);
    libinput_dispatchable =
        std::make_shared<md::ReadableFd>(
            Fd{IntOwnedFd{libinput_get_fd(lib.get())}}, [this]{process_input_events();}
        );
    udev_dispatchable =
        std::make_shared<DispatchableUDevMonitor>(
            *udev_context,
            [this](auto type, auto const& device)
            {
                switch(type)
                {
                case mu::Monitor::ADDED:
                {
                    if (device.devnode())
                    {
                        std::string devnode{device.devnode()};
                        // Libinput filters out anything without “event” as its name
                        if (strncmp(device.sysname(), "event", strlen("event")) != 0)
                        {
                            return;
                        }
                        if (pending_devices.count(device.devnum()) > 0 ||
                            device_watchers.count(device.devnum()) > 0)
                        {
                            // We're already handling this, ignore.
                            return;
                        }

                        pending_devices.insert(
                            {
                            device.devnum(),
                            console->acquire_device(
                                major(device.devnum()),
                                minor(device.devnum()),
                                [this, devnode = std::move(devnode), devnum = device.devnum()]
                                    (mir::Fd&& device_fd)
                                {
                                    device_fds.store_fd(devnode.c_str(), std::move(device_fd));

                                    libinput_path_add_device(lib.get(), devnode.c_str());
                                },
                                [this, devnum = device.devnum(), syspath = std::string{device.syspath()}]()
                                {
                                    for (auto const& input_device : devices)
                                    {
                                        auto device_udev = libinput_device_get_udev_device(input_device->device());

                                        if (device_udev)
                                        {
                                            if (syspath == udev_device_get_syspath(device_udev))
                                            {
                                                libinput_path_remove_device(input_device->device());
                                            }
                                        }
                                    }
                                    device_watchers.erase(devnum);
                                },
                                [this, devnum = device.devnum(), syspath = std::string{device.syspath()}]()
                                {
                                    for (auto const& input_device : devices)
                                    {
                                        auto device_udev = libinput_device_get_udev_device(input_device->device());

                                        if (device_udev)
                                        {
                                            if (syspath == udev_device_get_syspath(device_udev))
                                            {
                                                libinput_path_remove_device(input_device->device());
                                            }
                                        }
                                    }
                                    device_watchers.erase(devnum);
                                })});
                    }
                    break;
                }
                case mu::Monitor::REMOVED:
                {
                    for (auto const& input_device : this->devices)
                    {
                        auto device_udev = libinput_device_get_udev_device(input_device->device());

                        if (device_udev)
                        {
                            if (strcmp(device.syspath(), udev_device_get_syspath(device_udev)) == 0)
                            {
                                libinput_path_remove_device(input_device->device());
                                device_watchers.erase(device.devnum());
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            });
    platform_dispatchable->add_watch(udev_dispatchable);
    platform_dispatchable->add_watch(libinput_dispatchable);
    process_input_events();
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
        auto type = libinput_event_get_type(ev.get());
        auto device = libinput_event_get_device(ev.get());

        if (type == LIBINPUT_EVENT_DEVICE_ADDED)
        {
            device_added(device);
        }
        else if(type == LIBINPUT_EVENT_DEVICE_REMOVED)
        {
            device_removed(device);
        }
        else
        {
            auto dev = find_device(
                libinput_device_get_device_group(device));
            if (dev != end(devices))
                (*dev)->process_event(ev.get());
        }
    }
}

void mie::Platform::pause_for_config()
{
}

void mie::Platform::continue_after_config()
{
}

void mie::Platform::device_added(libinput_device* dev)
{
    auto device_ptr = make_libinput_device(lib, dev);

    log_info("Added %s", describe(dev).c_str());

    auto const devnum = udev_device_get_devnum(libinput_device_get_udev_device(device_ptr.get()));
    device_watchers.insert({devnum, pending_devices.at(devnum).get()});
    pending_devices.erase(devnum);

    auto device_it = find_device(libinput_device_get_device_group(device_ptr.get()));
    if (end(devices) != device_it)
    {
        (*device_it)->add_device_of_group(move(device_ptr));
        log_debug("Device %s is part of an already opened device group", libinput_device_get_sysname(dev));
        return;
    }

    try
    {
        devices.emplace_back(std::make_shared<mie::LibInputDevice>(report, move(device_ptr)));

        input_device_registry->add_device(devices.back());

        report->opened_input_device(libinput_device_get_sysname(dev), "evdev-input");
    } catch(...)
    {
        mir::log_error("Failure opening device %s", libinput_device_get_sysname(dev));
        report->failed_to_open_input_device(libinput_device_get_sysname(dev), "evdev-input");
    }
}

void mie::Platform::device_removed(libinput_device* dev)
{
    auto known_device_pos = find_device(libinput_device_get_device_group(dev));

    if (known_device_pos == end(devices))
        return;

    input_device_registry->remove_device(*known_device_pos);
    devices.erase(known_device_pos);

    log_info("Removed %s", describe(dev).c_str());
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

void mie::Platform::stop()
{
    if (udev_dispatchable)
    {
        platform_dispatchable->remove_watch(udev_dispatchable);
    }
    if (libinput_dispatchable)
    {
        platform_dispatchable->remove_watch(libinput_dispatchable);
    }
    while (!devices.empty())
    {
        input_device_registry->remove_device(devices.back());
        devices.pop_back();
    }
    libinput_dispatchable.reset();
    udev_dispatchable.reset();
    lib.reset();
}
