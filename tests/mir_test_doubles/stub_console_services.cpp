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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/test/doubles/stub_console_services.h"

#include "mir/fd.h"

#include <sys/sysmacros.h>
#include <fcntl.h>
#include <xf86drm.h>

namespace mtd = mir::test::doubles;

void mtd::StubConsoleServices::register_switch_handlers(
        mir::graphics::EventHandlerRegister&,
        std::function<bool()> const&,
        std::function<bool()> const&)
{
}

void mtd::StubConsoleServices::restore()
{
}

namespace
{
bool is_drm_device(int major, int /*minor*/)
{
    return major == 226;
}
}

std::future<std::unique_ptr<mir::Device>>
mtd::StubConsoleServices::acquire_device(
    int major, int minor,
    std::unique_ptr<mir::Device::Observer> observer)
{
    /* NOTE: This uses the behaviour that MockDRM will intercept any open() call
     * under /dev/dri/
     */
    auto devnode = devnode_for_devnum(major, minor);
    mir::Fd device_fd{::open(devnode, O_RDWR | O_CLOEXEC)};
    std::promise<std::unique_ptr<mir::Device>> promise;
    if (!is_drm_device(major, minor) || drmSetMaster(device_fd) == 0)
    {
        observer->activated(std::move(device_fd));
        // The Device is *just* a handle; there's no reason for anything to dereference it
        promise.set_value(nullptr);
    }
    else
    {
        promise.set_exception(
            std::make_exception_ptr(
                std::system_error{
                    errno,
                    std::system_category(),
                    "drmSetMaster failed"}));
    }

    return promise.get_future();
}

char const* mtd::StubConsoleServices::devnode_for_devnum(int major, int minor)
{
    auto const devnum = makedev(major, minor);
    auto& devnode = devnodes[devnum];

    if (devnode.empty())
    {
        if (is_drm_device(major, minor))
        {
            devnodes[devnum] = std::string{"/dev/dri/card"} + std::to_string(device_count);
            ++device_count;
        }
        else
        {
            devnodes[devnum] = "/dev/null";
        }
        devnode = devnodes[devnum];
    }
    return devnode.c_str();
}
