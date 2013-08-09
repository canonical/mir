/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_GRAPHICS_GBM_LINUX_VIRTUAL_TERMINAL_H_
#define MIR_GRAPHICS_GBM_LINUX_VIRTUAL_TERMINAL_H_

#include "virtual_terminal.h"

#include <memory>

#include <linux/vt.h>
#include <termios.h>
#include <unistd.h>

namespace mir
{
namespace graphics
{

class DisplayReport;

namespace gbm
{

class VTFileOperations
{
public:
    virtual ~VTFileOperations() = default;

    virtual int open(char const* pathname, int flags) = 0;
    virtual int close(int fd) = 0;
    virtual int ioctl(int d, int request, int val) = 0;
    virtual int ioctl(int d, int request, void* p_val) = 0;
    virtual int tcsetattr(int d, int acts, const struct termios *tcattr) = 0;
    virtual int tcgetattr(int d, struct termios *tcattr) = 0;

protected:
    VTFileOperations() = default;
    VTFileOperations(VTFileOperations const&) = delete;
    VTFileOperations& operator=(VTFileOperations const&) = delete;
};

class LinuxVirtualTerminal : public VirtualTerminal
{
public:
    LinuxVirtualTerminal(std::shared_ptr<VTFileOperations> const& fops,
                         int vt_number,
                         std::shared_ptr<DisplayReport> const& report);
    ~LinuxVirtualTerminal() noexcept(true);

    void set_graphics_mode();
    void register_switch_handlers(
        EventHandlerRegister& handlers,
        std::function<bool()> const& switch_away,
        std::function<bool()> const& switch_back);

private:
    class FDWrapper
    {
    public:
        FDWrapper(std::shared_ptr<VTFileOperations> const& fops, int fd)
            : fops{fops}, fd_{fd}
        {
        }
        ~FDWrapper() { if (fd_ >= 0) fops->close(fd_); }
        int fd() const { return fd_; }
    private:
        std::shared_ptr<VTFileOperations> const fops;
        int const fd_;
    };

    int find_active_vt_number();
    int open_vt(int vt_number);


    std::shared_ptr<VTFileOperations> const fops;
    std::shared_ptr<DisplayReport> const report;
    FDWrapper const vt_fd;
    int prev_kd_mode;
    struct vt_mode prev_vt_mode;
    int prev_tty_mode;
    struct termios prev_tcattr;
    bool active;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_LINUX_VIRTUAL_TERMINAL_H_ */
