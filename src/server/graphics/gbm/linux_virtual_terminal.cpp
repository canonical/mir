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

#include "linux_virtual_terminal.h"
#include "mir/main_loop.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <csignal>

#include <fcntl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

namespace mgg = mir::graphics::gbm;

namespace
{

int find_active_vt_number()
{
    static std::vector<std::string> const paths{"/dev/tty", "/dev/tty0"};
    int active_vt{-1};

    for (auto& p : paths)
    {
        auto fd = open(p.c_str(), O_RDONLY, 0);
        if (fd < 0)
            fd = open(p.c_str(), O_WRONLY, 0);

        if (fd >= 0)
        {
            struct vt_stat vts;
            auto status = ioctl(fd, VT_GETSTATE, &vts);
            close(fd);

            if (status >= 0)
            {
                active_vt = vts.v_active;
                break;
            }
        }
    }

    if (active_vt < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to find the current VT"));
    }

    return active_vt;
}

int open_vt(int vt_number)
{
    std::stringstream vt_path_stream;
    vt_path_stream << "/dev/tty" << vt_number;

    std::string const active_vt_path{vt_path_stream.str()};

    auto vt_fd = open(active_vt_path.c_str(), O_RDONLY | O_NDELAY, 0);

    if (vt_fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to open current VT"))
                    << boost::errinfo_file_name(active_vt_path)
                    << boost::errinfo_errno(errno));
    }

    return vt_fd;
}

}

mgg::LinuxVirtualTerminal::LinuxVirtualTerminal()
    : vt_fd{open_vt(find_active_vt_number())},
      prev_kd_mode{0},
      prev_vt_mode(),
      active{true}
{
    if (ioctl(vt_fd.fd(), KDGETMODE, &prev_kd_mode) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get current VT mode"))
                    << boost::errinfo_errno(errno));
    }

    if (ioctl(vt_fd.fd(), VT_GETMODE, &prev_vt_mode) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get the current VT")) << boost::errinfo_errno(errno));
    }
}

mgg::LinuxVirtualTerminal::~LinuxVirtualTerminal() noexcept(true)
{
    if (vt_fd.fd() > 0)
    {
        ioctl(vt_fd.fd(), KDSETMODE, prev_kd_mode);
        ioctl(vt_fd.fd(), VT_SETMODE, &prev_vt_mode);
    }
}

void mgg::LinuxVirtualTerminal::set_graphics_mode()
{
    if (ioctl(vt_fd.fd(), KDSETMODE, KD_GRAPHICS) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set VT to graphics mode"))
                    << boost::errinfo_errno(errno));
    }
}

void mgg::LinuxVirtualTerminal::register_switch_handlers(
    MainLoop& main_loop,
    std::function<void()> const& switch_away,
    std::function<void()> const& switch_back)
{
    main_loop.register_signal_handler(
        {SIGUSR1},
        [this, switch_away, switch_back](int)
        {
            active = !active;
            if (active)
            {
                static int const allow_switch{2};
                switch_back();
                ioctl(vt_fd.fd(), VT_RELDISP, allow_switch);
            }
            else
            {
                switch_away();
                ioctl(vt_fd.fd(), VT_RELDISP, VT_ACKACQ);
            }
        });

    struct vt_mode vtm
    {
        VT_PROCESS,
        0,
        SIGUSR1,
        SIGUSR1,
        0
    };

    if (ioctl(vt_fd.fd(), VT_SETMODE, &vtm) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set the current VT mode"))
                    << boost::errinfo_errno(errno));
    }
}
