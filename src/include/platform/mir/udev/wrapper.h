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

#ifndef MIR_UDEV_WRAPPER_H_
#define MIR_UDEV_WRAPPER_H_

#include <functional>
#include <memory>
#include <libudev.h>

namespace mir
{
namespace udev
{

class Device;
class Enumerator;

class Context
{
public:
    Context();
    ~Context() noexcept;

    Context(Context const&) = delete;
    Context& operator=(Context const&) = delete;

    std::shared_ptr<Device> device_from_syspath(std::string const& syspath);

    ::udev* ctx() const;

private:
    ::udev* const context;
};

class Device
{
public:
    virtual ~Device() = default;

    Device(Device const&) = delete;
    Device& operator=(Device const&) = delete;

    virtual char const* subsystem() const = 0;
    virtual char const* devtype() const = 0;
    virtual char const* devpath() const = 0;
    virtual char const* devnode() const = 0;
    virtual char const* property(char const *name) const = 0;
    virtual dev_t devnum() const = 0;
    virtual char const* sysname() const = 0;
    virtual bool initialised() const = 0;
    virtual char const* syspath() const = 0;
protected:
    Device() = default;
};

bool operator==(Device const& lhs, Device const& rhs);
bool operator!=(Device const& lhs, Device const& rhs);

class Enumerator
{
public:
    Enumerator(std::shared_ptr<Context> const& ctx);
    ~Enumerator() noexcept;

    Enumerator(Enumerator const&) = delete;
    Enumerator& operator=(Enumerator const&) = delete;

    void scan_devices();

    void match_subsystem(std::string const& subsystem);
    void match_parent(Device const& parent);
    void match_sysname(std::string const& sysname);

    class iterator :
        public std::iterator<std::input_iterator_tag, Device>
    {
    public:
        iterator& operator++();
        iterator operator++(int);

        bool operator==(iterator const& rhs) const;
        bool operator!=(iterator const& rhs) const;

        Device const& operator*() const;
        Device const* operator->() const;
    private:
        friend class Enumerator;

        iterator ();
        iterator (std::shared_ptr<Context> const& ctx, udev_list_entry* entry);

        void increment();

        std::shared_ptr<Context> ctx;
        udev_list_entry* entry;

        std::shared_ptr<Device> current;
    };

    iterator begin();
    iterator end();

private:
    std::shared_ptr<Context> const ctx;
    udev_enumerate* const enumerator;
    bool scanned;
};

class Monitor
{
public:
    enum EventType {
        ADDED,
        REMOVED,
        CHANGED,
    };

    Monitor(const Context& ctx);
    ~Monitor() noexcept;

    Monitor(Monitor const&) = delete;
    Monitor& operator=(Monitor const&) = delete;

    void enable(void);
    int fd(void) const;

    void filter_by_subsystem(std::string const& subsystem);
    void filter_by_subsystem_and_type(std::string const& subsystem, std::string const& devtype);

    void process_events(std::function<void(EventType, Device const&)> const& handler) const;
    void process_event(std::function<void(EventType, Device const&)> const& handler) const;

private:
    udev_monitor* const monitor;
    bool enabled;
};

}
}
#endif // MIR_UDEV_WRAPPER_H_
