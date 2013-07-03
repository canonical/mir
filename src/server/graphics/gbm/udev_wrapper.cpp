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

mgg::UdevDevice::UdevDevice(udev* ctx, std::string const& syspath)
{
    dev = udev_device_new_from_syspath(ctx, syspath.c_str());
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

/////////////////////
//    UdevIterator
/////////////////////

mgg::UdevIterator::UdevIterator () : entry(nullptr)
{
}

mgg::UdevIterator::UdevIterator (udev* ctx, udev_list_entry* entry) :
    ctx(ctx),
    entry(entry),
    current(std::make_shared<UdevDevice>(ctx, udev_list_entry_get_name(entry)))
{
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


mgg::UdevEnumerator::UdevEnumerator(udev* ctx) :
    enumerator(udev_enumerate_new(ctx))
{
}

mgg::UdevEnumerator::~UdevEnumerator() noexcept
{
    udev_enumerate_unref(enumerator);
}

void mgg::UdevEnumerator::scan_devices()
{
    udev_enumerate_scan_devices(enumerator);
}

mgg::UdevIterator mgg::UdevEnumerator::begin()
{
    return UdevIterator(udev_enumerate_get_udev(enumerator),
                        udev_enumerate_get_list_entry(enumerator));
}

mgg::UdevIterator mgg::UdevEnumerator::end()
{
    return UdevIterator();
}
