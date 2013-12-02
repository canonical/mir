/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <mir/udev_wrapper.h>
#include <libudev.h>
#include <cstring>
#include <boost/throw_exception.hpp>

/////////////////////
//    UdevDevice
/////////////////////

namespace
{
class UdevDeviceImpl : public mir::UdevDevice
{
public:
    UdevDeviceImpl(udev_device *dev);
    virtual ~UdevDeviceImpl() noexcept;
    virtual bool operator==(mir::UdevDevice const& rhs) const override;
    virtual bool operator!=(mir::UdevDevice const& rhs) const override;

    virtual char const * subsystem() const override;
    virtual char const * devtype() const override;
    virtual char const * devpath() const override;
    virtual char const * devnode() const override;

    udev_device* const dev;
};

UdevDeviceImpl::UdevDeviceImpl(udev_device *dev)
    : dev(dev)
{
    if (!dev)
	BOOST_THROW_EXCEPTION(std::runtime_error("Udev device does not exist"));

    udev_ref(udev_device_get_udev(dev));
}

UdevDeviceImpl::~UdevDeviceImpl() noexcept
{
    udev_unref(udev_device_get_udev(dev));
    udev_device_unref(dev);
}

bool UdevDeviceImpl::operator==(UdevDevice const& rhs) const
{
    // The device path is unique
    return strcmp(devpath(), rhs.devpath()) == 0;
}

bool UdevDeviceImpl::operator!=(UdevDevice const& rhs) const
{
    return !(*this == rhs);
}

char const * UdevDeviceImpl::subsystem() const
{
    return udev_device_get_subsystem(dev);
}

char const * UdevDeviceImpl::devtype() const
{
    return udev_device_get_devtype(dev);
}

char const * UdevDeviceImpl::devpath() const
{
    return udev_device_get_devpath(dev);
}

char const * UdevDeviceImpl::devnode() const
{
    return udev_device_get_devnode(dev);
}
}

////////////////////////
//    UdevEnumerator
////////////////////////

mir::UdevEnumerator::iterator::iterator () : entry(nullptr)
{
}

mir::UdevEnumerator::iterator::iterator (std::shared_ptr<UdevContext> const& ctx, udev_list_entry* entry) :
    ctx(ctx),
    entry(entry)
{
    if (entry)
        current = ctx->device_from_syspath(udev_list_entry_get_name(entry));
}

void mir::UdevEnumerator::iterator::increment()
{
    entry = udev_list_entry_get_next(entry);
    if (entry)
    {
        try
        {
            current = ctx->device_from_syspath(udev_list_entry_get_name(entry));
        }
        catch (std::runtime_error)
        {
            // The UdevDevice throws a runtime_error if the device does not exist
            // This can happen if it has been removed since the iterator was created.
            // If this happens, move on to the next device.
            increment();
        }
    }
}

mir::UdevEnumerator::iterator& mir::UdevEnumerator::iterator::operator++()
{
    increment();
    return *this;
}

mir::UdevEnumerator::iterator mir::UdevEnumerator::iterator::operator++(int)
{
    auto tmp = *this;
    increment();
    return tmp;
}

bool mir::UdevEnumerator::iterator::operator==(mir::UdevEnumerator::iterator const& rhs) const
{
    return this->entry == rhs.entry;
}

bool mir::UdevEnumerator::iterator::operator!=(mir::UdevEnumerator::iterator const& rhs) const
{
    return !(*this == rhs);
}

mir::UdevDevice const& mir::UdevEnumerator::iterator::operator*() const
{
    return *current;
}

mir::UdevEnumerator::UdevEnumerator(std::shared_ptr<UdevContext> const& ctx) :
    ctx(ctx),
    enumerator(udev_enumerate_new(ctx->ctx())),
    scanned(false)
{
}

mir::UdevEnumerator::~UdevEnumerator() noexcept
{
    udev_enumerate_unref(enumerator);
}

void mir::UdevEnumerator::scan_devices()
{
    udev_enumerate_scan_devices(enumerator);
    scanned = true;
}

void mir::UdevEnumerator::match_subsystem(std::string const& subsystem)
{
    udev_enumerate_add_match_subsystem(enumerator, subsystem.c_str());
}

void mir::UdevEnumerator::match_parent(mir::UdevDevice const& parent)
{
    udev_enumerate_add_match_parent(enumerator, dynamic_cast<UdevDeviceImpl const&>(parent).dev);
}

void mir::UdevEnumerator::match_sysname(std::string const& sysname)
{
    udev_enumerate_add_match_sysname(enumerator, sysname.c_str());
}

mir::UdevEnumerator::iterator mir::UdevEnumerator::begin()
{
    if (!scanned)
        BOOST_THROW_EXCEPTION(std::logic_error("Attempted to iterate over udev devices without first scanning"));

    return iterator(ctx,
                    udev_enumerate_get_list_entry(enumerator));
}

mir::UdevEnumerator::iterator mir::UdevEnumerator::end()
{
    return iterator();
}

///////////////////
//   UdevContext
///////////////////

mir::UdevContext::UdevContext()
    : context(udev_new())
{
    if (!context)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create udev context"));
}
mir::UdevContext::~UdevContext() noexcept
{
    udev_unref(context);
}

std::shared_ptr<mir::UdevDevice> mir::UdevContext::device_from_syspath(std::string const& syspath)
{
    return std::make_shared<UdevDeviceImpl>(udev_device_new_from_syspath(context, syspath.c_str()));
}

udev* mir::UdevContext::ctx() const
{
    return context;
}

///////////////////
//   UdevMonitor
///////////////////
mir::UdevMonitor::UdevMonitor(mir::UdevContext const& ctx)
    : monitor(udev_monitor_new_from_netlink(ctx.ctx(), "udev")),
      enabled(false)
{
    if (!monitor)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create udev_monitor"));

    udev_ref(udev_monitor_get_udev(monitor));
}

mir::UdevMonitor::~UdevMonitor() noexcept
{
    udev_unref(udev_monitor_get_udev(monitor));
    udev_monitor_unref(monitor);
}

void mir::UdevMonitor::enable(void)
{
    udev_monitor_enable_receiving(monitor);
    enabled = true;
}

static mir::UdevMonitor::EventType action_to_event_type(const char* action)
{
    if (strcmp(action, "add") == 0)
        return mir::UdevMonitor::EventType::ADDED;
    if (strcmp(action, "remove") == 0)
        return mir::UdevMonitor::EventType::REMOVED;
    if (strcmp(action, "change") == 0)
        return mir::UdevMonitor::EventType::CHANGED;
    BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Unknown udev action encountered: ") + action));
}

void mir::UdevMonitor::process_events(std::function<void(mir::UdevMonitor::EventType,
                                                         mir::UdevDevice const&)> const& handler) const
{
    udev_device *dev;
    do
    {
        dev = udev_monitor_receive_device(const_cast<udev_monitor*>(monitor));
        if (dev != nullptr)
            handler(action_to_event_type(udev_device_get_action(dev)), UdevDeviceImpl(dev));
    } while (dev != nullptr);
}

int mir::UdevMonitor::fd(void) const
{
    return udev_monitor_get_fd(const_cast<udev_monitor*>(monitor));
}

void mir::UdevMonitor::filter_by_subsystem_and_type(std::string const& subsystem, std::string const& devtype)
{
    udev_monitor_filter_add_match_subsystem_devtype(monitor, subsystem.c_str(), devtype.c_str());
    if (enabled)
        udev_monitor_filter_update(monitor);
}
