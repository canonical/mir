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
#include <boost/throw_exception.hpp>

namespace mgg = mir::graphics::gbm;

/////////////////////
//    UdevDevice
/////////////////////

mgg::UdevDevice::UdevDevice(UdevContext const& ctx, std::string const& syspath)
{
    dev = udev_device_new_from_syspath(ctx.ctx, syspath.c_str());
    if (!dev)
        BOOST_THROW_EXCEPTION(std::runtime_error("Udev device does not exist"));
}

mgg::UdevDevice::~UdevDevice() noexcept
{
    udev_device_unref(dev);
}

mgg::UdevDevice::UdevDevice(UdevDevice const& copy)
{
    dev = udev_device_ref(copy.dev);
}

mgg::UdevDevice& mgg::UdevDevice::operator=(UdevDevice const &rhs) noexcept
{
    udev_device_unref(dev);
    dev = udev_device_ref(rhs.dev);
    return *this;
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

/////////////////////
//    UdevIterator
/////////////////////

mgg::UdevIterator::UdevIterator () : entry(nullptr)
{
}

mgg::UdevIterator::UdevIterator (UdevContext const& ctx, udev_list_entry* entry) :
    ctx(ctx),
    entry(entry)
{
    if (entry)
        current = std::make_shared<UdevDevice>(ctx, udev_list_entry_get_name(entry));
}

void mgg::UdevIterator::increment()
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

bool mgg::UdevIterator::equal(mgg::UdevIterator const& other) const
{
    return this->entry == other.entry;
}

mgg::UdevDevice& mgg::UdevIterator::dereference() const
{
    return *current;
}

////////////////////////
//    UdevEnumerator
////////////////////////


mgg::UdevEnumerator::UdevEnumerator(UdevContext const& ctx) :
    ctx(ctx),
    enumerator(udev_enumerate_new(ctx.ctx)),
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

void mgg::UdevEnumerator::add_match_subsystem(std::string const& subsystem)
{
    udev_enumerate_add_match_subsystem(enumerator, subsystem.c_str());
}

void mgg::UdevEnumerator::match_parent(mgg::UdevDevice const& parent)
{
    udev_enumerate_add_match_parent(enumerator, parent.dev);
}

mgg::UdevIterator mgg::UdevEnumerator::begin()
{
    if (!scanned)
        BOOST_THROW_EXCEPTION(std::logic_error("Attempted to iterate over udev devices without first scanning"));

    return UdevIterator(ctx,
                        udev_enumerate_get_list_entry(enumerator));
}

mgg::UdevIterator mgg::UdevEnumerator::end()
{
    return UdevIterator();
}

///////////////////
//   UdevContext
///////////////////

mgg::UdevContext::UdevContext()
{
    ctx = udev_new();
    if (!ctx)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create udev context"));
}
mgg::UdevContext::~UdevContext() noexcept
{
    udev_unref(ctx);
}

mgg::UdevContext::UdevContext(UdevContext const& copy) :
    ctx(udev_ref(copy.ctx))
{
}

mgg::UdevContext& mgg::UdevContext::operator=(UdevContext const& rhs) noexcept
{
    udev_unref(ctx);
    ctx = udev_ref(rhs.ctx);
    return *this;
}
