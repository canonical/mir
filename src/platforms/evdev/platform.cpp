/*
 * Copyright © Canonical Ltd.
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
 */

#include "platform.h"
#include "libinput_device.h"
#include "libinput_ptr.h"
#include "fd_store.h"

#include "mir/udev/wrapper.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"
#include "mir/console_services.h"

#include "mir/input/input_device_registry.h"
#include "mir/input/input_report.h"
#include "mir/fd.h"
#include "mir/raii.h"

#define MIR_LOG_COMPONENT "evdev-input"
#include "mir/log.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <libinput.h>

#include <version>
#include <format>
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

#ifdef ARG_DEBUGGING
    for (auto entry = udev_device_get_properties_list_entry(udev_dev.get()); entry; entry = udev_list_entry_get_next(entry))
    {
        mir::log_debug("{arg} property '%s=%s'", udev_list_entry_get_name(entry), udev_list_entry_get_value(entry));
    }
    for (auto entry = udev_device_get_tags_list_entry(udev_dev.get()); entry; entry = udev_list_entry_get_next(entry))
    {
        mir::log_debug("{arg} tag '%s=%s'", udev_list_entry_get_name(entry), udev_list_entry_get_value(entry));
    }
    for (auto entry = udev_device_get_sysattr_list_entry(udev_dev.get()); entry; entry = udev_list_entry_get_next(entry))
    {
        mir::log_debug("{arg} sysattr '%s=%s'", udev_list_entry_get_name(entry), udev_list_entry_get_value(entry));
    }
#endif

    auto const vendor = libinput_device_get_id_vendor(dev);
    auto const product = libinput_device_get_id_product(dev);
    desc += std::format(" [{:0>4x}:{:0>4x}]", vendor, product);

    if (auto const name = libinput_device_get_name(dev))
    {
        desc += ": " + std::string(name);
    }

    for (auto const key : {/*"ID_MODEL", "ID_VENDOR_ID", "ID_MODEL_ID",*/ "ID_PATH"})
    {
        if (char const *const value = udev_device_get_property_value(udev_dev.get(), key))
        {
            desc += ": " + std::string(key) + "=" + value;
        }
    }

    return desc;
}

class DispatchableUDevMonitor : public md::Dispatchable
{
public:
    DispatchableUDevMonitor(
        mu::Context& context,
        std::function<void(mu::Monitor::EventType, mu::Device const&)> on_event)
        : monitor(context),
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
        // The mu::Monitor owns the file descriptor; don't close it
        // (cf: https://github.com/MirServer/mir/issues/684 )
        return mir::Fd{mir::IntOwnedFd{monitor.fd()}};
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

namespace
{
class InputDeviceObserver : public mir::Device::Observer
{
public:
    InputDeviceObserver(
        std::shared_ptr<md::ActionQueue> action_queue,
        mie::FdStore& device_fds,
        std::shared_ptr<::libinput>& lib,
        mu::Device const& device,
        std::vector<std::shared_ptr<mie::LibInputDevice>>& devices,
        std::unordered_map<dev_t, std::future<std::unique_ptr<mir::Device>>>& pending_devices,
        std::unordered_map<dev_t, std::unique_ptr<mir::Device>>& device_watchers)
        : action_queue{std::move(action_queue)},
          device_fds{device_fds},
          lib{lib},
          devnum{device.devnum()},
          devnode{device.devnode()},
          syspath{device.syspath()},
          devices{devices},
          pending_devices{pending_devices},
          device_watchers{device_watchers}
    {
        /*
         * We take everything shared with Platform (except action_queue) by reference.
         *
         * The only actions this class takes is to queue up functions on the action_queue.
         * These functions can freely use the referenced members of mie::Platform as
         * the Platform is guaranteed to be alive when dispatch() is called on it.
         *
         * Likewise, there no threadsafety issues as everything is done from the dispatch
         * thread.
         */
    }

    void activated(mir::Fd&& device_fd) override
    {
        action_queue->enqueue(
            [
                &device_fds = device_fds,
                fd = std::move(device_fd),
                &lib = lib,
                devnode = devnode,
                devnum = devnum,
                &pending_devices = pending_devices,
                &device_watchers = device_watchers
            ]() mutable
            {
                device_fds.store_fd(devnode.c_str(), std::move(fd));

                auto pending_iter = pending_devices.find(devnum);
                if (pending_iter != pending_devices.end())
                {
                    device_watchers.emplace(devnum, pending_iter->second.get());
                    pending_devices.erase(pending_iter);
                }

                if (!libinput_path_add_device(lib.get(), devnode.c_str()))
                {
                    /* Oops, libinput didn't want this after all.
                     * libinput will have closed the FD as a part of failing to add
                     * the device, so we only need to release our Device handle.
                     */
                    device_watchers.erase(devnum);
                }
            });
    }

    void suspended() override
    {
        action_queue->enqueue(
            [
                &devices = devices,
                syspath = syspath
            ]()
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
            });
    }

    void removed() override
    {
        action_queue->enqueue(
            [
                &devices = devices,
                syspath = syspath
            ]()
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
            });
    }

private:
    std::shared_ptr<md::ActionQueue> const action_queue;

    mie::FdStore& device_fds;
    std::shared_ptr<::libinput>& lib;
    dev_t const devnum;
    std::string const devnode;
    std::string const syspath;
    std::vector<std::shared_ptr<mie::LibInputDevice>>& devices;
    std::unordered_map<dev_t, std::future<std::unique_ptr<mir::Device>>>& pending_devices;
    std::unordered_map<dev_t, std::unique_ptr<mir::Device>>& device_watchers;
};
}

void mie::Platform::start()
{
    lib = make_libinput(&device_fds);
    libinput_dispatchable =
        std::make_shared<md::ReadableFd>(
            Fd{IntOwnedFd{libinput_get_fd(lib.get())}}, [this]{process_input_events();}
        );
    action_queue = std::make_shared<md::ActionQueue>();
    udev_dispatchable =
        std::make_shared<DispatchableUDevMonitor>(
            *udev_context,
            [this](auto type, auto const& device)
        {
            using namespace std::string_literals;
            auto event_type= "unknown"s;
            try
            {
                switch (type)
                {
                case mu::Monitor::ADDED:
                {
                    event_type = "ADDED"s;
                    if (device.devnode())
                    {
                        /*
                         * What if… madness?
                         *
                         * When we get notified from the mu::Monitor (ie: hotplug rather
                         * than on-start enumeration) under (at least) umockdev the
                         * udev_device is missing some properties. *Notably*,
                         * devices.devnum() is always 0.
                         *
                         * Work around this (umockdev?) bug by requesting a new
                         * Device from the syspath of the one we've been notified about.
                         */
                        auto ctx = std::make_shared<mu::Context>();
                        auto workaround_device = ctx->device_from_syspath(device.syspath());

                        std::string devnode{workaround_device->devnode()};
                        // Libinput filters out anything without “event” as its name
                        if (strncmp(workaround_device->sysname(), "event", strlen("event")) != 0)
                        {
                            return;
                        }
                        if (pending_devices.count(workaround_device->devnum()) > 0 ||
                            device_watchers.count(workaround_device->devnum()) > 0)
                        {
                            // We're already handling this, ignore.
                            return;
                        }

                        pending_devices.emplace(
                            workaround_device->devnum(),
                            console->acquire_device(
                                major(workaround_device->devnum()),
                                minor(workaround_device->devnum()),
                                std::make_unique<InputDeviceObserver>(
                                    action_queue,
                                    device_fds,
                                    lib,
                                    *workaround_device,
                                    devices,
                                    pending_devices,
                                    device_watchers)));
                    }
                    break;
                }
                case mu::Monitor::REMOVED:
                {
                    event_type = "REMOVED"s;
                    for (auto const& input_device : this->devices)
                    {
                        auto device_udev = libinput_device_get_udev_device(input_device->device());

                        if (device_udev)
                        {
                            if (strcmp(device.syspath(), udev_device_get_syspath(device_udev)) == 0)
                            {
                                libinput_path_remove_device(input_device->device());
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
            catch (std::exception const&)
            {
                auto const message = "Failed to handle UDev " + event_type + " event for " + device.syspath();
                log(logging::Severity::warning, MIR_LOG_COMPONENT, std::current_exception(), message);
            }
        });

    platform_dispatchable->add_watch(udev_dispatchable);
    platform_dispatchable->add_watch(libinput_dispatchable);
    platform_dispatchable->add_watch(action_queue);
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
            auto dev = find_device(device);
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

    auto device_it = find_device(device_ptr.get());
    if (end(devices) != device_it)
    {
        log_debug("Device %s is an already opened device", libinput_device_get_sysname(dev));
        return;
    }

    try
    {
        devices.emplace_back(std::make_shared<mie::LibInputDevice>(report, std::move(device_ptr)));

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
    auto known_device_pos = find_device(dev);

    if (known_device_pos == end(devices))
        return;

    auto const udev_device = libinput_device_get_udev_device(dev);
    input_device_registry->remove_device(*known_device_pos);
    devices.erase(known_device_pos);
    if (udev_device)
    {
        /* libinput documents that input devices might not have udev devices
         * In practise all our devices *will*, because we only add devices from udev events,
         * but let's not crash if not!
         */
        device_watchers.erase(udev_device_get_devnum(udev_device));
        udev_device_unref(udev_device);
    }

    log_info("Removed %s", describe(dev).c_str());
}

auto mie::Platform::find_device(libinput_device* dev) -> decltype(devices)::iterator
{
    return std::find_if(
        begin(devices),
        end(devices),
        [dev](auto const& item)
        {
            return dev == item->device();
        });
}

void mie::Platform::stop()
{
    // This must only be called from the dispatch thread, so this doesn't race
    device_watchers.clear();
    pending_devices.clear();

    if (action_queue)
    {
        platform_dispatchable->remove_watch(action_queue);
    }
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
    action_queue.reset();
    lib.reset();
}
