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

#ifndef MIR_GRAPHICS_GBM_UDEV_WRAPPER_H_
#define MIR_GRAPHICS_GBM_UDEV_WRAPPER_H_

#include <memory>
#include <libudev.h>
#include <boost/iterator/iterator_facade.hpp>

namespace mir
{
namespace graphics
{
namespace gbm
{


class UdevDevice
{
public:
    UdevDevice(udev* ctx, std::string const& syspath);
    ~UdevDevice() noexcept;

    UdevDevice(UdevDevice const& copy);
    UdevDevice& operator=(UdevDevice const &rhs) noexcept;

    char const* subsystem() const;
private:
    udev_device* dev;   
};

class UdevIterator : 
    public boost::iterator_facade<
                                  UdevIterator,
                                  UdevDevice,
                                  boost::forward_traversal_tag
                                 >
{
public:
    UdevIterator ();
    UdevIterator (udev* ctx, udev_list_entry* entry);

private:
    friend class boost::iterator_core_access;

    void increment();
    bool equal(UdevIterator const& other) const;
    UdevDevice& dereference() const;

    udev *ctx;
    udev_list_entry* entry;

    std::shared_ptr<UdevDevice> current;
};

class UdevEnumerator
{
public:
    UdevEnumerator(udev* ctx);
    ~UdevEnumerator() noexcept;

    void scan_devices();

    void add_match_subsystem(std::string const& subsystem);

    UdevIterator begin();
    UdevIterator end();

private:
    udev_enumerate* enumerator;
};

}
}
}
#endif // MIR_GRAPHICS_GBM_UDEV_WRAPPER_H_
