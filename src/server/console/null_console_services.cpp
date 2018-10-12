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

#include "null_console_services.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/throw_exception.hpp>

#include <sstream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

mir::NullConsoleDevice::NullConsoleDevice(VTFileOperations& fops,
                                          std::unique_ptr<mir::Device::Observer> observer,
                                          std::string const& dev)
    : fops{fops}, observer{std::move(observer)}, dev{dev}
{
}

void mir::NullConsoleDevice::on_activated()
{
    observer->activated(mir::Fd(fops.open(dev.c_str(), O_RDWR | O_CLOEXEC)));
}

void mir::NullConsoleDevice::on_suspended()
{
    observer->suspended();
}

void mir::NullConsoleServices::register_switch_handlers(graphics::EventHandlerRegister&,
                                                        std::function<bool()> const&,
                                                        std::function<bool()> const&)
{
    // do nothing since we do not switch away
}

void mir::NullConsoleServices::restore()
{
    // no need to restore because we were never gone
}

mir::NullConsoleServices::NullConsoleServices(std::shared_ptr<VTFileOperations> const& vtops) : fops(vtops)
{
}

std::unique_ptr<mir::VTSwitcher> mir::NullConsoleServices::create_vt_switcher()
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"NullConsoleServices does not support VT switching"}));
}

std::future<std::unique_ptr<mir::Device>> mir::NullConsoleServices::acquire_device(
    int major, int minor, std::unique_ptr<mir::Device::Observer> observer)
{
    std::stringstream filename;
    filename << "/sys/dev/char/" << major << ":" << minor << "/uevent";
    mir::Fd const fd{open(filename.str().c_str(), O_RDONLY | O_CLOEXEC)};

    if (fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((boost::enable_error_info(std::system_error{
                                   errno, std::system_category(), "Failed to open /sys file to discover devnode"})
                               << boost::errinfo_file_name(filename.str())));
    }

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
        auto device = std::make_unique<mir::NullConsoleDevice>(*fops, std::move(observer), devnode);
        device->on_activated();
        device_promise.set_value(std::move(device));
    }
    catch (std::exception const&)
    {
        device_promise.set_exception(std::current_exception());
    }

    return device_promise.get_future();
}
