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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "udev_video_devices.h"
#include "mir/graphics/event_handler_register.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <memory>

namespace mgg = mir::graphics::gbm;

mgg::UdevVideoDevices::UdevVideoDevices(udev* udev_ctx)
    : udev_ctx{udev_ctx}
{
}

void mgg::UdevVideoDevices::register_change_handler(
    EventHandlerRegister& handlers,
        std::function<void()> const& change_handler)
{
    auto monitor = std::shared_ptr<udev_monitor>(
                       udev_monitor_new_from_netlink(udev_ctx, "udev"),
                       [](udev_monitor* m) { if (m) udev_monitor_unref(m); });
    if (!monitor)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create udev_monitor"));

    udev_monitor_filter_add_match_subsystem_devtype(monitor.get(), "drm", "drm_minor");

    handlers.register_fd_handler(
        {udev_monitor_get_fd(monitor.get())},
        [change_handler, monitor](int)
        {
            auto dev = std::unique_ptr<udev_device,std::function<void(udev_device*)>>(
                           udev_monitor_receive_device(monitor.get()),
                           [](udev_device* d) { if (d) udev_device_unref(d); });
            if (dev)
                change_handler();
        });

    udev_monitor_enable_receiving(monitor.get());
}
