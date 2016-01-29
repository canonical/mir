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
#include "mir/assert_module_entry_point.h"

#include "mir/input/input_device_registry.h"
#include "mir/input/input_device.h"
#include "mir/input/input_report.h"
#include "mir/fd.h"

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
    udev_device *udev_dev = libinput_device_get_udev_device(dev);
    std::string desc(udev_device_get_devnode(udev_dev));

    char const * const model = udev_device_get_property_value(udev_dev, "ID_MODEL");
    if (model)
        desc += ": " + std::string(model);

    // Yes, we could use std::replace but this will compile smaller and faster
    for (auto &c : desc)
        if (c == '_') c = ' ';

    return desc;
}

} // namespace

mie::Platform::Platform(std::shared_ptr<InputDeviceRegistry> const& registry,
                              std::shared_ptr<InputReport> const& report,
                              std::unique_ptr<udev::Context>&& udev_context) :
    report(report),
    udev_context(std::move(udev_context)),
    input_device_registry(registry),
    platform_dispatchable{std::make_shared<md::MultiplexingDispatchable>()},
    lib{make_libinput(this->udev_context->ctx())},
    libinput_dispatchable{
        std::make_shared<md::ReadableFd>(
            Fd{IntOwnedFd{libinput_get_fd(lib.get())}},
            [this](){process_input_events();}
            )}
{

}

std::shared_ptr<mir::dispatch::Dispatchable> mie::Platform::dispatchable()
{
    return platform_dispatchable;
}

void mie::Platform::start()
{
    platform_dispatchable->add_watch(libinput_dispatchable);
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
            device_added(device);
        else if(type == LIBINPUT_EVENT_DEVICE_REMOVED)
            device_removed(device);
        else
        {
            auto dev = find_device(
                libinput_device_get_device_group(device));
            if (dev != end(devices))
                (*dev)->process_event(ev.get());
        }
    }
}

void mie::Platform::device_added(libinput_device* dev)
{
    auto device_ptr = make_libinput_device(lib, dev);

    // libinput might refuse to open certain devices nodes like /dev/input/mice
    // or ignore devices with odd evdev bits/capabilities set
    if (!device_ptr)
    {
        return;
    }

    log_info("Added %s", describe(dev).c_str());

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
    platform_dispatchable->remove_watch(libinput_dispatchable);
    while (!devices.empty())
    {
        input_device_registry->remove_device(devices.back());
        devices.pop_back();
    }
}
