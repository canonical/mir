/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "udev_wrapper.h"
#include <libudev.h>
#include <string.h>
#include <boost/throw_exception.hpp>

namespace mgg = mir::graphics::gbm;

/////////////////////
//    UdevDevice
/////////////////////

mgg::UdevDevice::UdevDevice(std::shared_ptr<UdevContext> const& ctx, std::string const& syspath)
{
    dev = udev_device_new_from_syspath(ctx->ctx(), syspath.c_str());
    if (!dev)
        BOOST_THROW_EXCEPTION(std::runtime_error("Udev device does not exist"));
}

mgg::UdevDevice::~UdevDevice() noexcept
{
    udev_device_unref(dev);
}

bool mgg::UdevDevice::operator==(UdevDevice const& rhs) const
{
    // The device path is unique
    return !strcmp(devpath(), rhs.devpath());
}

bool mgg::UdevDevice::operator!=(UdevDevice const& rhs) const
{
    return !(*this == rhs);
}

char const* mgg::UdevDevice::subsystem() const
{
    return udev_device_get_subsystem(dev);
}

char const* mgg::UdevDevice::devtype() const
{
    return udev_device_get_devtype(dev);
}

char const* mgg::UdevDevice::devpath() const
{
    return udev_device_get_devpath(dev);
}

char const* mgg::UdevDevice::devnode() const
{
    return udev_device_get_devnode(dev);
}

udev_device const* mgg::UdevDevice::device() const
{
    return dev;
}

////////////////////////
//    UdevEnumerator
////////////////////////

mgg::UdevEnumerator::iterator::iterator () : entry(nullptr)
{
}

mgg::UdevEnumerator::iterator::iterator (std::shared_ptr<UdevContext> const& ctx, udev_list_entry* entry) :
    ctx(ctx),
    entry(entry)
{
    if (entry)
        current = std::make_shared<UdevDevice>(ctx, udev_list_entry_get_name(entry));
}

void mgg::UdevEnumerator::iterator::increment()
{
    entry = udev_list_entry_get_next(entry);
    if (entry)
    {
        try
        {
            current = std::make_shared<UdevDevice>(ctx, udev_list_entry_get_name(entry));
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

mgg::UdevEnumerator::iterator& mgg::UdevEnumerator::iterator::operator++()
{
    increment();
    return *this;
}

mgg::UdevEnumerator::iterator mgg::UdevEnumerator::iterator::operator++(int)
{
    auto tmp = *this;
    increment();
    return tmp;
}

bool mgg::UdevEnumerator::iterator::operator==(mgg::UdevEnumerator::iterator const& rhs) const
{
    return this->entry == rhs.entry;
}

bool mgg::UdevEnumerator::iterator::operator!=(mgg::UdevEnumerator::iterator const& rhs) const
{
    return !(*this == rhs);
}

mgg::UdevDevice const& mgg::UdevEnumerator::iterator::operator*() const
{
    return *current;
}

mgg::UdevEnumerator::UdevEnumerator(std::shared_ptr<UdevContext> const& ctx) :
    ctx(ctx),
    enumerator(udev_enumerate_new(ctx->ctx())),
    scanned(false)
{
}

mgg::UdevEnumerator::~UdevEnumerator() noexcept
{
    udev_enumerate_unref(enumerator);
}

void mgg::UdevEnumerator::scan_devices()
{
    udev_enumerate_scan_devices(enumerator);
    scanned = true;
}

void mgg::UdevEnumerator::match_subsystem(std::string const& subsystem)
{
    udev_enumerate_add_match_subsystem(enumerator, subsystem.c_str());
}

void mgg::UdevEnumerator::match_parent(mgg::UdevDevice const& parent)
{
    // Need to const_cast<> as this increases the udev_device's reference count
    udev_enumerate_add_match_parent(enumerator, const_cast<udev_device *>(parent.device()));
}

void mgg::UdevEnumerator::match_sysname(std::string const& sysname)
{
    udev_enumerate_add_match_sysname(enumerator, sysname.c_str());
}

mgg::UdevEnumerator::iterator mgg::UdevEnumerator::begin()
{
    if (!scanned)
        BOOST_THROW_EXCEPTION(std::logic_error("Attempted to iterate over udev devices without first scanning"));

    return iterator(ctx,
                    udev_enumerate_get_list_entry(enumerator));
}

mgg::UdevEnumerator::iterator mgg::UdevEnumerator::end()
{
    return iterator();
}

///////////////////
//   UdevContext
///////////////////

mgg::UdevContext::UdevContext()
{
    context = udev_new();
    if (!context)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create udev context"));
}
mgg::UdevContext::~UdevContext() noexcept
{
    udev_unref(context);
}

udev* mgg::UdevContext::ctx()
{
    return context;
}
