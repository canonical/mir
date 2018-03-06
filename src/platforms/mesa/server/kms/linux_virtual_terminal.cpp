/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "linux_virtual_terminal.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/event_handler_register.h"
#include "mir/fd.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <csignal>

#include <linux/vt.h>
#include <linux/kd.h>
#include <fcntl.h>

namespace mgm = mir::graphics::mesa;

mgm::LinuxVirtualTerminal::LinuxVirtualTerminal(std::shared_ptr<VTFileOperations> const& fops,
    std::unique_ptr<PosixProcessOperations> pops,
    int vt_number,
    std::shared_ptr<DisplayReport> const& report)
    : fops{fops},
      pops{std::move(pops)},
      report{report},
      vt_fd{fops, open_vt(vt_number)},
      prev_kd_mode{0},
      prev_vt_mode(),
      prev_tty_mode(),
      prev_tcattr(),
      active{true}
{
    struct termios tcattr;
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
                std::runtime_error("Failed to get the current VT"))
                    << boost::errinfo_errno(errno));
    }

    if (fops->ioctl(vt_fd.fd(), KDGKBMODE, &prev_tty_mode) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get the current TTY mode"))
                    << boost::errinfo_errno(errno));
    }
    if (fops->ioctl(vt_fd.fd(), KDSKBMODE, K_OFF) < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to mute keyboard"))
                    << boost::errinfo_errno(errno));
    }

    fops->tcgetattr(vt_fd.fd(), &prev_tcattr);
    tcattr = prev_tcattr;
    tcattr.c_iflag = IGNPAR | IGNBRK;
    cfsetispeed(&tcattr, B9600);
    tcattr.c_oflag = 0;
    cfsetospeed(&tcattr, B9600);
    tcattr.c_cflag = CREAD | CS8;
    tcattr.c_lflag = 0;
    tcattr.c_cc[VTIME] = 0;
    tcattr.c_cc[VMIN] = 1;
    fops->tcsetattr(vt_fd.fd(), TCSANOW, &tcattr);

    if (fops->ioctl(vt_fd.fd(), KDSETMODE, KD_GRAPHICS) < 0)
    {
        try
        {
            restore();
        }
        catch(...)
        {
        }

        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set VT to graphics mode"))
                << boost::errinfo_errno(errno));
    }
}

mgm::LinuxVirtualTerminal::~LinuxVirtualTerminal() noexcept(true)
{
    restore();
}

void mgm::LinuxVirtualTerminal::register_switch_handlers(
    EventHandlerRegister& handlers,
    std::function<bool()> const& switch_away,
    std::function<bool()> const& switch_back)
{
    handlers.register_signal_handler(
        {SIGUSR1},
        make_module_ptr<std::function<void(int)>>(
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
            }));

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

void mgm::LinuxVirtualTerminal::restore()
{
    if (vt_fd.fd() >= 0)
    {
        fops->tcsetattr(vt_fd.fd(), TCSANOW, &prev_tcattr);
        fops->ioctl(vt_fd.fd(), KDSKBMODE, prev_tty_mode);
        fops->ioctl(vt_fd.fd(), KDSETMODE, prev_kd_mode);

        /*
         * Only restore the previous mode if it was VT_AUTO. VT_PROCESS mode is
         * always bound to the calling process, so "restoring" VT_PROCESS will
         * not work; it will just bind the notification signals to our process
         * again. Not "restoring" VT_PROCESS also ensures we don't mess up the
         * VT state of the previous controlling process, in case it had set
         * VT_PROCESS and we fail during setup.
         */
        if (prev_vt_mode.mode == VT_AUTO)
            fops->ioctl(vt_fd.fd(), VT_SETMODE, &prev_vt_mode);
    }
}


int mgm::LinuxVirtualTerminal::find_active_vt_number()
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

int mgm::LinuxVirtualTerminal::open_vt(int vt_number)
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

    if (activate)
    {
        // we should only try to create a new session in order to become the session
        // and group leader if we are not already the session leader
        if (pops->getpid() != pops->getsid(0))
        {
            if (pops->getpid() == pops->getpgid(0) && pops->setpgid(0, pops->getpgid(pops->getppid())) < 0)
            {
                BOOST_THROW_EXCEPTION(
                    boost::enable_error_info(
                        std::runtime_error("Failed to stop being a process group"))
                           << boost::errinfo_errno(errno));
            }

            /* become process group leader */
            if (pops->setsid() < 0)
            {
                BOOST_THROW_EXCEPTION(
                    boost::enable_error_info(
                        std::runtime_error("Failed to become session leader"))
                           << boost::errinfo_errno(errno));
            }
        }
    }

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

boost::unique_future<mir::Fd> mgm::LinuxVirtualTerminal::acquire_device(int major, int minor)
{
    std::stringstream filename;
    filename << "/sys/dev/char/" << major << ":" << minor << "/uevent";
    mir::Fd const fd{fops->open(filename.str().c_str(), O_RDONLY | O_CLOEXEC)};

    if (fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((boost::enable_error_info(std::system_error{
            errno,
            std::system_category(),
            "Failed to open /sys file to discover devnode"})
                << boost::errinfo_file_name(filename.str())));
    }

    auto const devnode =
        [](auto const& fd)
        {
            using namespace boost::iostreams;
            char line_buffer[1024];
            stream<file_descriptor_source> uevent{fd, file_descriptor_flags::never_close_handle};

            while (uevent.getline(line_buffer, sizeof(line_buffer)))
            {
                if (strncmp(line_buffer, "DEVNAME=", strlen("DEVNAME=")) == 0)
                {
                    return std::string{"/dev/"} + std::string{line_buffer + strlen("DEVNAME=")};
                }
            }
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to read DEVNAME"}));
        }(fd);

    boost::promise<mir::Fd> promise;
    auto dev_fd = mir::Fd{fops->open(devnode.c_str(), O_RDWR | O_CLOEXEC)};
    if (dev_fd != mir::Fd::invalid)
    {
        promise.set_value(std::move(dev_fd));
    }
    else
    {
        promise.set_exception(
            std::make_exception_ptr(
                boost::enable_error_info(
                    std::system_error{
                        errno,
                        std::system_category(),
                        "Failed to open device node"})
                    << boost::errinfo_file_name(devnode.c_str())
                ));
    }

    return promise.get_future();
}
