/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@gmail.com>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "minimal_console_services.h"

#include "mir/log.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/throw_exception.hpp>

#include <sstream>

#include <xf86drm.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

// Older versions of libdrm do not provides drmIsMaster()
#ifndef MIR_LIBDRM_HAS_IS_MASTER
#include <drm.h>

bool drmIsMaster(int fd)
{
    struct drm_mode_mode_cmd cmd;

    ::memset(&cmd, 0, sizeof cmd);
    /* Set an invalid connector_id to ensure that ATTACHMODE errors with
     * EINVAL in the unlikely event someone feels like calling this on a
     * kernel prior to 3.9. */
    cmd.connector_id = -1;

    if (drmIoctl(fd, DRM_IOCTL_MODE_ATTACHMODE, &cmd) != -1)
    {
        /* On 3.9 ATTACHMODE was changed to drm_noop, and so will succeed
         * iff we've got a master fd */
        return true;
    }

    return errno == EINVAL;
}
#endif

mir::MinimalConsoleDevice::MinimalConsoleDevice(std::unique_ptr<mir::Device::Observer> observer)
    : observer{std::move(observer)}
{
}

void mir::MinimalConsoleDevice::on_activated(mir::Fd&& fd)
{
    observer->activated(std::move(fd));
}

void mir::MinimalConsoleDevice::on_suspended()
{
    observer->suspended();
}

void mir::MinimalConsoleServices::register_switch_handlers(
    graphics::EventHandlerRegister&,
    std::function<bool()> const&,
    std::function<bool()> const&)
{
    // do nothing since we do not switch away
}

void mir::MinimalConsoleServices::restore()
{
    // no need to restore because we were never gone
}

mir::MinimalConsoleServices::MinimalConsoleServices()
{
}

std::unique_ptr<mir::VTSwitcher> mir::MinimalConsoleServices::create_vt_switcher()
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"MinimalConsoleServices does not support VT switching"}));
}

namespace
{
mir::Fd checked_open(char const* filename, int flags, char const* exception_msg)
{
    mir::Fd fd{::open(filename, flags)};
    if (fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((
            boost::enable_error_info(
                std::system_error{
                    errno,
                    std::system_category(),
                    exception_msg})
                << boost::errinfo_file_name(filename)));
    }
    return fd;
}
}

std::future<std::unique_ptr<mir::Device>> mir::MinimalConsoleServices::acquire_device(
    int major, int minor, std::unique_ptr<mir::Device::Observer> observer)
{
    std::stringstream filename;
    filename << "/sys/dev/char/" << major << ":" << minor << "/uevent";
    auto const fd = checked_open(
        filename.str().c_str(),
        O_RDONLY | O_CLOEXEC,
        "Failed to open /sys file to discover devnode");

    auto const devnode = [](auto const& fd) {
        using namespace boost::iostreams;
        char line_buffer[1024];
        stream<file_descriptor_source> uevent{fd, file_descriptor_flags::never_close_handle};

        while (uevent.getline(line_buffer, sizeof(line_buffer)))
        {
            if (strncmp(line_buffer, "DEVNAME=", strlen("DEVNAME=")) == 0)
            {
                return std::string{"/dev/"} + std::string{line_buffer + strlen("DEVNAME=")};
            }
        }
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to read DEVNAME"}));
    }(fd);

    std::promise<std::unique_ptr<mir::Device>> device_promise;
    try
    {
        /* Ideally we would check DRM nodes for drmMaster, but there doesn't appear to be
         * a way to do that!
         */
        auto const open_flags = O_RDWR | O_CLOEXEC |
            ((major == 226) ? 0 : O_NONBLOCK);

        auto fd = checked_open(
            devnode.c_str(),
            open_flags,
            "Failed to open device node");

        if (major == 226 && !drmIsMaster(fd))
        {
            // We haven't received implicit master, so explicitly try to acquire master
            if (auto ret = drmSetMaster(fd))
            {
                BOOST_THROW_EXCEPTION((
                    std::system_error{
                        -ret,
                        std::system_category(),
                        "Failed to acquire DRM master"}));
            }
        }

        auto device = std::make_unique<mir::MinimalConsoleDevice>(std::move(observer));
        device->on_activated(std::move(fd));
        device_promise.set_value(std::move(device));
    }
    catch (std::exception const&)
    {
        device_promise.set_exception(std::current_exception());
    }

    return device_promise.get_future();
}
