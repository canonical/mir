/*
 * Copyright © 2018 Canonical Ltd.
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

#include "utils.h"

#include "mir/graphics/egl_error.h"

#include <sys/stat.h>

#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_file_name.hpp>

namespace mg = mir::graphics;

dev_t mg::eglstream::devnum_for_device(EGLDeviceEXT device)
{
    auto const device_path = eglQueryDeviceStringEXT(device, EGL_DRM_DEVICE_FILE_EXT);
    if (!device_path)
    {
        BOOST_THROW_EXCEPTION(
            mg::egl_error("Failed to determine DRM device node path from EGLDevice"));
    }

    struct stat info;
    if (stat(device_path, &info) == -1)
    {
        BOOST_THROW_EXCEPTION((
            boost::enable_error_info(
                std::system_error{
                    errno,
                    std::system_category(),
                    "Failed to stat device node"})
                << boost::errinfo_file_name(device_path)));
    }

    if ((info.st_mode & S_IFMT) != S_IFCHR)
    {
        BOOST_THROW_EXCEPTION((
            boost::enable_error_info(
                std::runtime_error{"Queried device node is unexpectedly not a char device"})
                << boost::errinfo_file_name(device_path)));
    }

    return info.st_rdev;
}

auto mg::eglstream::parse_nvidia_version(char const* gl_version) -> std::experimental::optional<VersionInfo>
{
    /* Parses the GL_VERSION output of the NVIDIA binary drivers, which has the form
     * $GL_VERSION NVIDIA $NVIDIA_DRIVER_MAJOR.$NVIDIA_DRIVER_MINOR
     *
     * We only want the NVIDIA driver version.
     */
    VersionInfo info;
    auto const matches = ::sscanf(gl_version, "%*d.%*d NVIDIA %d.%d", &info.major, &info.minor);
    if (matches == 2)
    {
        // If we successfully parsed into the two bits, return them.
        return info;
    }
    return {};
}
