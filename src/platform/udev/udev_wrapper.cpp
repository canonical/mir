/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/udev/wrapper.h"
#include <libudev.h>
#include <cstring>
#include <boost/throw_exception.hpp>

namespace mu = mir::udev;

/////////////////////
//    Device
/////////////////////

namespace
{
class DeviceImpl : public mu::Device
{
public:
    DeviceImpl(udev_device *dev);
    virtual ~DeviceImpl() noexcept;

    virtual char const* subsystem() const override;
    virtual char const* devtype() const override;
    virtual char const* devpath() const override;
    virtual char const* devnode() const override;
    virtual char const* property(char const*) const override;
    virtual dev_t devnum() const override;
    virtual char const* sysname() const override;
    virtual bool initialised() const override;
    virtual char const* syspath() const override;

    udev_device* const dev;
};

DeviceImpl::DeviceImpl(udev_device *dev)
    : dev(dev)
{
    if (!dev)
        BOOST_THROW_EXCEPTION(std::runtime_error("Udev device does not exist"));

    udev_ref(udev_device_get_udev(dev));
}

DeviceImpl::~DeviceImpl() noexcept
{
    udev_unref(udev_device_get_udev(dev));
    udev_device_unref(dev);
}

char const* DeviceImpl::subsystem() const
{
    return udev_device_get_subsystem(dev);
}

char const* DeviceImpl::devtype() const
{
    return udev_device_get_devtype(dev);
}

char const* DeviceImpl::devpath() const
{
    return udev_device_get_devpath(dev);
}

char const* DeviceImpl::devnode() const
{
    return udev_device_get_devnode(dev);
}

char const* DeviceImpl::sysname() const
{
    return udev_device_get_sysname(dev);
}

char const* DeviceImpl::property(char const* name) const
{
    return udev_device_get_property_value(dev, name);
}

dev_t DeviceImpl::devnum() const
{
    return udev_device_get_devnum(dev);
}

bool DeviceImpl::initialised() const
{
    return static_cast<bool>(udev_device_get_is_initialized(dev));
}

char const* DeviceImpl::syspath() const
{
    return udev_device_get_syspath(dev);
}
}

bool mu::operator==(mu::Device const& lhs, mu::Device const& rhs)
{
    // The device path is unique
    return strcmp(lhs.devpath(), rhs.devpath()) == 0;
}

bool mu::operator!=(mu::Device const& lhs, mu::Device const& rhs)
{
    return !(lhs == rhs);
}



////////////////////////
//    Enumerator
////////////////////////

mu::Enumerator::iterator::iterator () : entry(nullptr)
{
}

mu::Enumerator::iterator::iterator (std::shared_ptr<Context> const& ctx, udev_list_entry* entry) :
    ctx(ctx),
    entry(entry)
{
    if (entry)
        current = ctx->device_from_syspath(udev_list_entry_get_name(entry));
}

void mu::Enumerator::iterator::increment()
{
    entry = udev_list_entry_get_next(entry);
    if (entry)
    {
        try
        {
            current = ctx->device_from_syspath(udev_list_entry_get_name(entry));
        }
        catch (std::runtime_error const&)
        {
            // The Device throws a runtime_error if the device does not exist
            // This can happen if it has been removed since the iterator was created.
            // If this happens, move on to the next device.
            increment();
        }
    }
    else
    {
        current.reset();
    }
}

mu::Enumerator::iterator& mu::Enumerator::iterator::operator++()
{
    increment();
    return *this;
}

mu::Enumerator::iterator mu::Enumerator::iterator::operator++(int)
{
    auto tmp = *this;
    increment();
    return tmp;
}

bool mu::Enumerator::iterator::operator==(mu::Enumerator::iterator const& rhs) const
{
    return this->entry == rhs.entry;
}

bool mu::Enumerator::iterator::operator!=(mu::Enumerator::iterator const& rhs) const
{
    return !(*this == rhs);
}

mu::Device const& mu::Enumerator::iterator::operator*() const
{
    return *current;
}

mu::Device const* mu::Enumerator::iterator::operator->() const
{
    return current.get();
}

mu::Enumerator::Enumerator(std::shared_ptr<Context> const& ctx) :
    ctx(ctx),
    enumerator(udev_enumerate_new(ctx->ctx())),
    scanned(false)
{
}

mu::Enumerator::~Enumerator() noexcept
{
    udev_enumerate_unref(enumerator);
}

void mu::Enumerator::scan_devices()
{
    udev_enumerate_scan_devices(enumerator);
    scanned = true;
}

void mu::Enumerator::match_subsystem(std::string const& subsystem)
{
    udev_enumerate_add_match_subsystem(enumerator, subsystem.c_str());
}

void mu::Enumerator::match_parent(mu::Device const& parent)
{
    udev_enumerate_add_match_parent(enumerator, dynamic_cast<DeviceImpl const&>(parent).dev);
}

void mu::Enumerator::match_sysname(std::string const& sysname)
{
    udev_enumerate_add_match_sysname(enumerator, sysname.c_str());
}

mu::Enumerator::iterator mu::Enumerator::begin()
{
    if (!scanned)
        BOOST_THROW_EXCEPTION(std::logic_error("Attempted to iterate over udev devices without first scanning"));

    return iterator(ctx,
                    udev_enumerate_get_list_entry(enumerator));
}

mu::Enumerator::iterator mu::Enumerator::end()
{
    return iterator();
}

///////////////////
//   Context
///////////////////

mu::Context::Context()
    : context(udev_new())
{
    if (!context)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create udev context"));
}

mu::Context::~Context() noexcept
{
    udev_unref(context);
}

std::shared_ptr<mu::Device> mu::Context::device_from_syspath(std::string const& syspath)
{
    return std::make_shared<DeviceImpl>(udev_device_new_from_syspath(context, syspath.c_str()));
}

udev* mu::Context::ctx() const
{
    return context;
}

///////////////////
//   Monitor
///////////////////
mu::Monitor::Monitor(mu::Context const& ctx)
    : monitor(udev_monitor_new_from_netlink(ctx.ctx(), "udev")),
      enabled(false)
{
    if (!monitor)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create udev_monitor"));

    udev_ref(udev_monitor_get_udev(monitor));
}

mu::Monitor::~Monitor() noexcept
{
    udev_unref(udev_monitor_get_udev(monitor));
    udev_monitor_unref(monitor);
}

void mu::Monitor::enable(void)
{
    udev_monitor_enable_receiving(monitor);
    enabled = true;
}

static mu::Monitor::EventType action_to_event_type(const char* action)
{
    if (strcmp(action, "add") == 0)
        return mu::Monitor::EventType::ADDED;
    if (strcmp(action, "remove") == 0)
        return mu::Monitor::EventType::REMOVED;
    if (strcmp(action, "change") == 0)
        return mu::Monitor::EventType::CHANGED;
    BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Unknown udev action encountered: ") + action));
}

void mu::Monitor::process_events(std::function<void(mu::Monitor::EventType,
                                                    mu::Device const&)> const& handler) const
{
    udev_device *dev;
    do
    {
        dev = udev_monitor_receive_device(monitor);
        if (dev != nullptr)
            handler(action_to_event_type(udev_device_get_action(dev)), DeviceImpl(dev));
    } while (dev != nullptr);
}

void mu::Monitor::process_event(
    std::function<void(mu::Monitor::EventType, mu::Device const&)> const& handler) const
{
    udev_device *dev;
    dev = udev_monitor_receive_device(monitor);
    if (dev != nullptr)
        handler(action_to_event_type(udev_device_get_action(dev)), DeviceImpl(dev));
}

int mu::Monitor::fd(void) const
{
    return udev_monitor_get_fd(monitor);
}

void mu::Monitor::filter_by_subsystem(std::string const& subsystem)
{
    udev_monitor_filter_add_match_subsystem_devtype(monitor, subsystem.c_str(), nullptr);
    if (enabled)
        udev_monitor_filter_update(monitor);
}

void mu::Monitor::filter_by_subsystem_and_type(std::string const& subsystem, std::string const& devtype)
{
    udev_monitor_filter_add_match_subsystem_devtype(monitor, subsystem.c_str(), devtype.c_str());
    if (enabled)
        udev_monitor_filter_update(monitor);
}
