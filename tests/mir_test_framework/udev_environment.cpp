/*
 * Copyright © 2013 Canonical Ltd.
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
 * Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_test_framework/udev_environment.h"

#include <umockdev.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>


namespace mtf = mir::mir_test_framework;

namespace
{
std::string binary_path()
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
}

mtf::UdevEnvironment::UdevEnvironment()
    : recordings_path(binary_path() + "/udev_recordings")
{
    testbed = umockdev_testbed_new();
}

mtf::UdevEnvironment::~UdevEnvironment() noexcept
{
    g_object_unref(testbed);
}

std::string mtf::UdevEnvironment::add_device(char const* subsystem,
                                             char const* name,
                                             char const* parent,
                                             std::initializer_list<char const*> attributes,
                                             std::initializer_list<char const*> properties)
{
    std::vector<char const*> attrib(attributes);
    std::vector<char const*> props(properties);

    attrib.push_back(nullptr);
    props.push_back(nullptr);

    gchar* syspath =  umockdev_testbed_add_devicev(testbed,
                                                   subsystem,
                                                   name,
                                                   parent,
                                                   const_cast<gchar**>(attrib.data()),
                                                   const_cast<gchar**>(props.data()));

    if (syspath == nullptr)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create mock udev device"));

    std::string retval(syspath);
    g_free(syspath);
    return retval;
}

void mtf::UdevEnvironment::remove_device(std::string const& device_path)
{
    umockdev_testbed_uevent(testbed, device_path.c_str(), "remove");
    umockdev_testbed_remove_device(testbed, device_path.c_str());
}

void mtf::UdevEnvironment::emit_device_changed(std::string const& device_path)
{
    umockdev_testbed_uevent(testbed, device_path.c_str(), "change");
}

void mtf::UdevEnvironment::add_standard_device(std::string const& name)
{
    auto descriptor_filename = recordings_path + "/" + name + ".umockdev";
    GError* err = nullptr;
    if (!umockdev_testbed_add_from_file(testbed, descriptor_filename.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to create mock udev device: ") +
                                                 err->message));
    }
    
    auto ioctls_filename = recordings_path + "/" + name + ".ioctl";
    struct stat sb;
    if (stat(ioctls_filename.c_str(), &sb) == 0)
    {
        if (S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode))
        {
            if (!umockdev_testbed_load_ioctl(testbed, NULL, ioctls_filename.c_str(), &err))
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to load ioctl recording: ") +
                                                         err->message));
            }
        }
    }
}
