/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_INPUT_EVDEV_FDSTORE_H_
#define MIR_INPUT_EVDEV_FDSTORE_H_

#include <unordered_map>
#include <sys/sysmacros.h>

#include "mir/fd.h"
#include <string>

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
    void remove_fd(int fd);

private:
    FdStore(FdStore const&) = delete;
    FdStore& operator=(FdStore const&) = delete;

    std::unordered_map<std::string, mir::Fd> fds;

    // LibInputPtr calls remove_fd() for touchpads on suspend but
    // continues to use take_fd() to access the fd after resume!
    // As a workaround, we remember the last removed fd and reinstate
    // it if asked for.
    //                  https://github.com/canonical/mir/issues/1612
    std::pair<std::string, mir::Fd> removed;
};

}
}
}

#endif //MIR_INPUT_EVDEV_FDSTORE_H_
