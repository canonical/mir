/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "ioctl_vt_switcher.h"

#include <boost/exception/enable_error_info.hpp>
#include <boost/exception/info.hpp>

#include <sys/ioctl.h>
#include <sys/vt.h>

mir::console::IoctlVTSwitcher::IoctlVTSwitcher(mir::Fd vt_fd)
    : vt_fd{std::move(vt_fd)}
{
}

void mir::console::IoctlVTSwitcher::switch_to(
    int vt_number,
    std::function<void(std::exception const&)> error_handler)
{
    if (ioctl(vt_fd, VT_ACTIVATE, vt_number) == -1)
    {
        auto const error = boost::enable_error_info(
            std::system_error{
                errno,
                std::system_category(),
                "Kernel request to change VT switch failed"})
                << boost::throw_line(__LINE__)
                << boost::throw_function(__PRETTY_FUNCTION__)
                << boost::throw_file(__FILE__);
        error_handler(error);
    }
}
