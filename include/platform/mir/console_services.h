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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_PLATFORM_CONSOLE_SERVICES_H_
#define MIR_PLATFORM_CONSOLE_SERVICES_H_

#include <functional>
#include <future>

namespace mir
{
class Fd;

namespace graphics
{
class EventHandlerRegister;
}

/**
 * Fully-opaque handle to a device.
 */
class Device
{
public:
    Device() = default;
    /**
     * Destructor. No further callbacks will be triggered once destruction has completed.
     */
    virtual ~Device() = default;

    Device(Device const&) = delete;
    Device& operator=(Device const&) = delete;

    class Observer
    {
    public:
        virtual ~Observer() = default;

        /**
         * Called when a the device is activated - ie: is ready to be used.
         *
         * For DRM devices, device_fd will already be DRM master, and all fds will refer to the
         * same underlying file descriptor; you can safely use only the first fd you receive.
         *
         * For other devices each call of OnDeviceActivated will typically return a new file
         * descriptor; only the most recently received file descriptor is guaranteed to be usable.
         *
         * \param device_fd [in] The file descriptor for the newly-active device.
         */
        virtual void activated(mir::Fd&& device_fd) = 0;

        /**
         * Called when the device is, or is about to be, suspended.
         *
         * For DRM devices the device fd will become functional again at the next OnDeviceActivated
         * call.
         *
         * For other devices, the device fd may remain silenced; users should close their existing
         * fd and use device_fd from the next OnDeviceActivated call.
         */
        virtual void suspended() = 0;

        /**
         * Called when the device has been removed; no further callbacks will be generated.
         *
         * The only sensible thing to do in the callback is remove any Mir object associated with
         * the Device, and then destroy the Device.
         */
        virtual void removed() = 0;
    };
};

class ConsoleServices
{
public:
    virtual ~ConsoleServices() = default;

    virtual void register_switch_handlers(
        graphics::EventHandlerRegister& handlers,
        std::function<bool()> const& switch_away,
        std::function<bool()> const& switch_back) = 0;
    virtual void restore() = 0;

    /**
     * Asynchronously acquire access to a device node.
     *
     * \param major             [in] major number of requested device node
     * \param minor             [in] minor number of requested device node
     * \param observer          [in] Observer that will receive notifications for this Device
     *
     * \return  A future that will resolve to a Device handle. Before the future resolves at least
     *          one of Observer::activated, Observer::suspended, or Observer::removed will have
     *          been called.
     *          The lifetime of the \param observer is guaranteed to be no longer than that of
     *          the returned std::unique_ptr<Device>, although it may end sooner if the
     *          implementation determines that no further events can be generated.
     */
    virtual std::future<std::unique_ptr<Device>> acquire_device(
        int major, int minor,
        std::unique_ptr<Device::Observer> observer) = 0;

protected:
    ConsoleServices() = default;
    ConsoleServices(ConsoleServices const&) = delete;
    ConsoleServices& operator=(ConsoleServices const&) = delete;
};

}

#endif /* MIR_PLATFORM_CONSOLE_SERVICES_H_*/
