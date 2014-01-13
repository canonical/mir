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

#ifndef MIR_GRAPHICS_MESA_UDEV_WRAPPER_H_
#define MIR_GRAPHICS_MESA_UDEV_WRAPPER_H_

#include <memory>
#include <libudev.h>
#include <mir/graphics/event_handler_register.h>

namespace mir
{
namespace graphics
{
namespace mesa
{


class UdevDevice;
class UdevEnumerator;

class UdevContext
{
public:
    UdevContext();
    ~UdevContext() noexcept;

    UdevContext(UdevContext const&) = delete;
    UdevContext& operator=(UdevContext const&) = delete;

    udev* ctx() const;

private:
    udev* const context;
};

class UdevDevice
{
public:
    UdevDevice(UdevContext const& ctx, std::string const& syspath);
    UdevDevice(udev_device *dev);
    ~UdevDevice() noexcept;

    UdevDevice(UdevDevice const&) = delete;
    UdevDevice& operator=(UdevDevice const&) = delete;

    bool operator==(UdevDevice const& rhs) const;
    bool operator!=(UdevDevice const& rhs) const;

    char const* subsystem() const;
    char const* devtype() const;
    char const* devpath() const;
    char const* devnode() const;

    udev_device const* device() const;
private:
    udev_device* const dev;
};

class UdevEnumerator
{
public:
    UdevEnumerator(std::shared_ptr<UdevContext> const& ctx);
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
        iterator (std::shared_ptr<UdevContext> const& ctx, udev_list_entry* entry);

        void increment();

        std::shared_ptr<UdevContext> ctx;
        udev_list_entry* entry;

        std::shared_ptr<UdevDevice> current;
    };

    iterator begin();
    iterator end();

private:
    std::shared_ptr<UdevContext> const ctx;
    udev_enumerate* const enumerator;
    bool scanned;
};

class UdevMonitor
{
public:
    enum EventType {
        ADDED,
        REMOVED,
        CHANGED,
    };

    UdevMonitor(const UdevContext &ctx);
    ~UdevMonitor() noexcept;

    void enable(void);
    int fd(void) const;

    void filter_by_subsystem_and_type(std::string const& subsystem, std::string const& devtype);

    void process_events(std::function<void(EventType, UdevDevice const&)> const& handler) const;

private:
    udev_monitor* const monitor;
    bool enabled;
};

}
}
}
#endif // MIR_GRAPHICS_MESA_UDEV_WRAPPER_H_
