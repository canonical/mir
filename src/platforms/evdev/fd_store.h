/*
 * Copyright © 2017 Canonical Ltd.
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

#ifndef MIR_INPUT_EVDEV_FDSTORE_H_
#define MIR_INPUT_EVDEV_FDSTORE_H_

#include <unordered_map>
#include <sys/sysmacros.h>

#include "mir/fd.h"

namespace mir
{
namespace input
{
namespace evdev
{
class FdStore
{
public:
    FdStore() = default;
    void store_fd(char const* path, mir::Fd&& fd);
    mir::Fd take_fd(char const* path);

private:
    FdStore(FdStore const&) = delete;
    FdStore& operator=(FdStore const&) = delete;

    std::unordered_map<std::string, mir::Fd> fds;
};

}
}
}

#endif //MIR_INPUT_EVDEV_FDSTORE_H_
