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


class UdevDevice;
class UdevEnumerator;

class UdevContext
{
public:
    UdevContext();
    ~UdevContext() noexcept;

    UdevContext(UdevContext const& copy);
    UdevContext& operator=(UdevContext const& rhs) noexcept;

    udev* ctx;
};

class UdevDevice
{
public:
    UdevDevice(UdevContext const& ctx, std::string const& syspath);
    ~UdevDevice() noexcept;

    UdevDevice(UdevDevice const& copy);
    UdevDevice& operator=(UdevDevice const &rhs) noexcept;

    bool operator==(UdevDevice const& rhs) const;
    bool operator!=(UdevDevice const& rhs) const;

    char const* subsystem() const;
    char const* devtype() const;
    char const* devpath() const;
    char const* devnode() const;

    friend class UdevEnumerator;
private:
    udev_device* dev;   
};

class UdevEnumerator
{
public:
    UdevEnumerator(UdevContext const& ctx);
    ~UdevEnumerator() noexcept;

    void scan_devices();

    void match_subsystem(std::string const& subsystem);
    void match_parent(UdevDevice const& parent);
    void match_sysname(std::string const& sysname);

    class iterator : 
        public std::iterator<std::input_iterator_tag, UdevDevice>
    {
    public:
        iterator& operator++();
        iterator operator++(int);

        bool operator==(iterator const& rhs) const;
        bool operator!=(iterator const& rhs) const;

        UdevDevice const& operator*() const;

    private:
        friend class UdevEnumerator;

        iterator ();
        iterator (UdevContext const& ctx, udev_list_entry* entry);

        void increment();

        UdevContext ctx;
        udev_list_entry* entry;

        std::shared_ptr<UdevDevice> current;
    };

    iterator begin();
    iterator end();

private:
    UdevContext ctx;
    udev_enumerate* enumerator;
    bool scanned;
};

}
}
}
#endif // MIR_GRAPHICS_GBM_UDEV_WRAPPER_H_
