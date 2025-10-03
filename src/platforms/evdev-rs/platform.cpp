/*
 * Copyright © Canonical Ltd.
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
 */

#include "platform.h"
#include "platform_bridge.h"

#include "mir/dispatch/dispatchable.h"
#include "rust/cxx.h"
#include "mir_platforms_evdev_rs/src/lib.rs.h"

namespace mi = mir::input;
namespace miers = mir::input::evdev_rs;
namespace md = mir::dispatch;

namespace
{
template <typename Impl>
class DeviceObserverWrapper : public miers::DeviceObserverWithFd
{
public:
    explicit DeviceObserverWrapper(rust::Box<Impl>&& box)
        : box(std::move(box)) {}

    void activated(mir::Fd&& device_fd) override
    {
        fd = device_fd;
        box->activated(device_fd);
    }

    void suspended() override
    {
        box->suspended();
    }

    void removed() override
    {
        box->removed();
    }

    std::optional<mir::Fd> raw_fd() const override
    {
        return fd;
    }

private:
    rust::Box<Impl> box;
    std::optional<mir::Fd> fd;
};

class DispatchableStub : public md::Dispatchable
{
public:
    mir::Fd watch_fd() const override
    {
        return mir::Fd{0};
    }

    bool dispatch(md::FdEvents) override
    {
        return false;
    }

    mir::dispatch::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }
};
}

class miers::Platform::Self
{
public:
    Self(Platform* platform, std::shared_ptr<ConsoleServices> const& console)
        : platform_impl(evdev_rs_create(std::make_shared<PlatformBridgeC>(platform, console))) {}

    rust::Box<PlatformRs> platform_impl;
};

miers::Platform::Platform(std::shared_ptr<ConsoleServices> const& console)
    : self(std::make_shared<Self>(this, console))
{
}

std::shared_ptr<mir::dispatch::Dispatchable> miers::Platform::dispatchable()
{
    return std::make_shared<DispatchableStub>();
}

void miers::Platform::start()
{
    self->platform_impl->start();
}

void miers::Platform::continue_after_config()
{
    self->platform_impl->continue_after_config();
}

void miers::Platform::pause_for_config()
{
    self->platform_impl->pause_for_config();
}

void miers::Platform::stop()
{
    self->platform_impl->stop();
}

std::unique_ptr<miers::DeviceObserverWithFd> miers::Platform::create_device_observer()
{
    return std::make_unique<DeviceObserverWrapper<DeviceObserverRs>>(
        self->platform_impl->create_device_observer());
}

