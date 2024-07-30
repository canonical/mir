/*
 * Copyright © Canonical Ltd.
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
 */

#include "ioctl_vt_switcher.h"
#include "mir/log.h"

#include <sys/ioctl.h>
#include <string.h>
#include <sys/vt.h>

mir::console::IoctlVTSwitcher::IoctlVTSwitcher(mir::Fd vt_fd)
    : vt_fd{std::move(vt_fd)}
{
}

void mir::console::IoctlVTSwitcher::switch_to(
    int vt_number)
{
    if (ioctl(vt_fd, VT_ACTIVATE, vt_number) == -1)
    {
        mir::log_error("%s:%d: Kernel request to change VT switch failed: %s", __FILE__, __LINE__, strerror(errno));
    }
}
