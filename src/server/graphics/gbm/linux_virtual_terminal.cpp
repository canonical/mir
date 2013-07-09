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

#include "linux_virtual_terminal.h"
#include "mir/graphics/display_report.h"
#include "mir/main_loop.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <csignal>

#include <linux/vt.h>
#include <linux/kd.h>
#include <fcntl.h>

namespace mgg = mir::graphics::gbm;

mgg::LinuxVirtualTerminal::LinuxVirtualTerminal(
    std::shared_ptr<VTFileOperations> const& fops,
    int vt_number,
    std::shared_ptr<DisplayReport> const& report)
    : fops{fops},
      report{report},
      vt_fd{fops, open_vt(vt_number)},
      prev_kd_mode{0},
      prev_vt_mode(),
      active{true}
{
    if (fops->ioctl(vt_fd.fd(), KDGETMODE, &prev_kd_mode) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get current VT mode"))
                    << boost::errinfo_errno(errno));
    }

    if (fops->ioctl(vt_fd.fd(), VT_GETMODE, &prev_vt_mode) < 0)
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
        fops->ioctl(vt_fd.fd(), KDSETMODE, prev_kd_mode);
        fops->ioctl(vt_fd.fd(), VT_SETMODE, &prev_vt_mode);
    }
}

void mgg::LinuxVirtualTerminal::set_graphics_mode()
{
    if (fops->ioctl(vt_fd.fd(), KDSETMODE, KD_GRAPHICS) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set VT to graphics mode"))
                    << boost::errinfo_errno(errno));
    }
}

void mgg::LinuxVirtualTerminal::register_switch_handlers(
    MainLoop& main_loop,
    std::function<bool()> const& switch_away,
    std::function<bool()> const& switch_back)
{
    main_loop.register_signal_handler(
        {SIGUSR1},
        [this, switch_away, switch_back](int)
        {
            if (!active)
            {
                if (!switch_back())
                    report->report_vt_switch_back_failure();
                fops->ioctl(vt_fd.fd(), VT_RELDISP, VT_ACKACQ);
                active = true;
            }
            else
            {
                static int const disallow_switch{0};
                static int const allow_switch{1};
                int action;

                if (switch_away())
                {
                    action = allow_switch;
                    active = false;
                }
                else
                {
                    action = disallow_switch;
                    report->report_vt_switch_away_failure();
                }

                fops->ioctl(vt_fd.fd(), VT_RELDISP, action);
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

    if (fops->ioctl(vt_fd.fd(), VT_SETMODE, &vtm) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set the current VT mode"))
                    << boost::errinfo_errno(errno));
    }
}

int mgg::LinuxVirtualTerminal::find_active_vt_number()
{
    static std::vector<std::string> const paths{"/dev/tty", "/dev/tty0"};
    int active_vt{-1};

    for (auto& p : paths)
    {
        auto fd = fops->open(p.c_str(), O_RDONLY);
        if (fd < 0)
            fd = fops->open(p.c_str(), O_WRONLY);

        if (fd >= 0)
        {
            struct vt_stat vts;
            auto status = fops->ioctl(fd, VT_GETSTATE, &vts);
            fops->close(fd);

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

int mgg::LinuxVirtualTerminal::open_vt(int vt_number)
{
    auto activate = true;
    if (vt_number <= 0)
    {
        vt_number = find_active_vt_number();
        activate = false;
    }

    std::stringstream vt_path_stream;
    vt_path_stream << "/dev/tty" << vt_number;

    std::string const active_vt_path{vt_path_stream.str()};

    auto vt_fd = fops->open(active_vt_path.c_str(), O_RDONLY | O_NDELAY);

    if (vt_fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to open current VT"))
                    << boost::errinfo_file_name(active_vt_path)
                    << boost::errinfo_errno(errno));
    }

    if (activate)
    {
        auto status = fops->ioctl(vt_fd, VT_ACTIVATE, vt_number);
        if (status < 0)
        {
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error("Failed to activate VT"))
                        << boost::errinfo_file_name(active_vt_path)
                        << boost::errinfo_errno(errno));
        }
        status = fops->ioctl(vt_fd, VT_WAITACTIVE, vt_number);
        if (status < 0)
        {
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error("Failed to wait for VT to become active"))
                        << boost::errinfo_file_name(active_vt_path)
                        << boost::errinfo_errno(errno));
        }
    }

    return vt_fd;
}
