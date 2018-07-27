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

#ifndef MIR_CONSOLE_IOCTL_VT_SWITCHER_H
#define MIR_CONSOLE_IOCTL_VT_SWITCHER_H

#include "mir/console_services.h"
#include "mir/fd.h"

namespace mir
{
namespace console
{
class IoctlVTSwitcher : public VTSwitcher
{
public:
    IoctlVTSwitcher(mir::Fd vt_fd);

    void switch_to(
        int vt_number,
        std::function<void(std::exception const&)> const& error_handler) override;

private:
    mir::Fd const vt_fd;
};
}
}

#endif //MIR_CONSOLE_IOCTL_VT_SWITCHER_H
