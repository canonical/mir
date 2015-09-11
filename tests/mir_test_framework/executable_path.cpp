/*
 * Copyright © 2013,2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *  Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *  Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_framework/executable_path.h"

#include <libgen.h>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/filesystem.hpp>

std::string mir_test_framework::executable_path()
{
    char buf[1024];
    auto tmp = readlink("/proc/self/exe", buf, sizeof buf);
    if (tmp < 0)
        BOOST_THROW_EXCEPTION(boost::enable_error_info(
                                  std::runtime_error("Failed to find our executable path"))
                              << boost::errinfo_errno(errno));
    if (tmp > static_cast<ssize_t>(sizeof(buf) - 1))
        BOOST_THROW_EXCEPTION(std::runtime_error("Path to executable is too long!"));
    buf[tmp] = '\0';
    return dirname(buf);
}

std::string mir_test_framework::library_path()
{
    return executable_path() + "/../lib";
}

std::string mir_test_framework::udev_recordings_path()
{
    std::string bin_path   = MIR_BUILD_PREFIX"/bin/udev_recordings";
    std::string share_path = MIR_INSTALL_PREFIX"/share/udev_recordings";

    if (boost::filesystem::exists(bin_path))
        return bin_path;
    else if (boost::filesystem::exists(share_path))
        return share_path;

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to find udev_recordings in standard search locations"));
}

std::string mir_test_framework::server_platform(std::string const& name)
{
    std::string libname{name};

    if (libname.find(".so") == std::string::npos)
        libname += ".so." MIR_SERVER_GRAPHICS_PLATFORM_ABI_STRING;

    for (auto const& option :
         {library_path() + "/server-modules/", library_path() + "/server-platform/", std::string(MIR_SERVER_PLATFORM_PATH) + '/'})
    {
        auto path_to_test = option + libname;
        if (boost::filesystem::exists(path_to_test))
            return path_to_test;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to find server platform in standard search locations"));
}

std::string mir_test_framework::client_platform(std::string const& name)
{
    std::string libname{name};

    if (libname.find(".so") == std::string::npos)
        libname += ".so." MIR_CLIENT_PLATFORM_ABI_STRING;

    for (auto const& option :
         {library_path() + "/client-modules/", library_path() + "/client-platform/", std::string(MIR_CLIENT_PLATFORM_PATH) + '/'})
    {
        auto path_to_test = option + libname;
        if (boost::filesystem::exists(path_to_test))
            return path_to_test;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to find server platform in standard search locations"));
}
