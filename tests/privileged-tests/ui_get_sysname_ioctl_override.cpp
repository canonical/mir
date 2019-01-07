/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

// This file provides a custom implementation for the UI_GET_SYSNAME ioctl
// request. Although libevent-uinput provides a fallback for platforms missing
// this ioctl, the fallback doesn't work well with some of our ARM devices. In
// particular, the fallback chooses the virtual device by checking for
// devices that were created while we were creating the device, like:
//
// a = now
// create device
// b = now
// ...
// if (a <= device_creation_time <= b) choose device
//
// This is reasonable, but on some devices, for unknown reasons, the creation
// time of the virtual device occasionally falls a bit before this interval.
//
// The custom implementation provided in this file uses similar logic but it's
// less strict in its timing requirements. It chooses the first device it finds
// that was created at most 2 seconds ago.
//
// Note that both approaches are racy in real conditions (e.g. other devices
// could have been created while we are creating our device, hence the need for
// the UI_GET_SYSNAME ioctl), but they are work reasonably well in a controlled
// testing environment.

#include <string>
#include <iostream>

#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>

namespace
{

time_t entry_creation_age(std::string const& parent_path, struct dirent* entry)
{
    auto const full = parent_path + "/" + entry->d_name;
    struct stat sb;
    stat(full.c_str(), &sb);
    return time(nullptr) - sb.st_ctime;
}

std::string find_new_virtual_device()
{
    std::string const path{"/sys/devices/virtual/input"};
    time_t const max_creation_age = 2;

    auto dp = opendir(path.c_str());
    if (dp == nullptr)
        return "";

    struct dirent* entry;
    while ((entry = readdir(dp)))
    {
        if (entry_creation_age(path, entry) <= max_creation_age)
            break;
    }

    closedir(dp);

    if (entry)
        return entry->d_name;
    else
        return "";
}

bool request_is_ui_get_sysname(unsigned long int request)
{
    return static_cast<unsigned long>(request & ~IOCSIZE_MASK) ==
           static_cast<unsigned long>(UI_GET_SYSNAME(0));
}

template<typename Param1>
auto request_param_type(int (* ioctl)(int, Param1, ...)) -> Param1;
}

extern "C" int ioctl(int fd, decltype(request_param_type(&ioctl)) request, ...) noexcept
{
    va_list vargs;
    va_start(vargs, request);

    using ioctl_func = decltype(&ioctl);
    static ioctl_func const real_ioctl =
        reinterpret_cast<ioctl_func>(dlsym(RTLD_NEXT, "ioctl"));

    void* arg = va_arg(vargs, void*);
    int result = real_ioctl(fd, request, arg);

    // If system doesn't support the UI_GET_SYSNAME ioctl,
    // try to service the request with our custom implementation
    if (result < 0 && request_is_ui_get_sysname(request))
    {
        std::cout << "Note: Using custom implementation for the UI_GET_SYSNAME ioctl, since system doesn't support it." << std::endl;
        auto const carg = static_cast<char*>(arg);
        int const size = _IOC_SIZE(request);
        auto const vdev = find_new_virtual_device();
        auto ncopied = vdev.copy(carg, size - 1);
        carg[ncopied] = '\0';

        result = vdev.empty() ? -1 : 0;
    }

    va_end(vargs);

    return result;
}
