/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_LINUX_VIRTUAL_TERMINAL_H_
#define MIR_GRAPHICS_GBM_LINUX_VIRTUAL_TERMINAL_H_

#include "virtual_terminal.h"

#include <linux/vt.h>
#include <unistd.h>

namespace mir
{
namespace graphics
{
namespace gbm
{

class LinuxVirtualTerminal : public VirtualTerminal
{
public:
    LinuxVirtualTerminal();
    ~LinuxVirtualTerminal() noexcept(true);

    void set_graphics_mode();
    void register_switch_handlers(
        MainLoop& main_loop,
        std::function<void()> const& switch_away,
        std::function<void()> const& switch_back);

private:
    class FDWrapper
    {
    public:
        FDWrapper(int fd) : fd_{fd} {}
        ~FDWrapper() { if (fd_ >= 0) close(fd_); }
        int fd() const { return fd_; }
    private:
        int const fd_;
    };

    FDWrapper const vt_fd;
    int prev_kd_mode;
    struct vt_mode prev_vt_mode;
    bool active;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_LINUX_VIRTUAL_TERMINAL_H_ */
