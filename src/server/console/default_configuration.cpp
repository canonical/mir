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

#include "mir/default_server_configuration.h"
#include "mir/options/configuration.h"
#include "mir/log.h"
#include "mir/emergency_cleanup.h"
#include "mir/glib_main_loop.h"
#include "mir/abnormal_exit.h"

#include "null_console_services.h"
#include "linux_virtual_terminal.h"
#include "logind_console_services.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <boost/exception/diagnostic_information.hpp>

namespace
{
struct RealVTFileOperations : public mir::VTFileOperations
{
    int open(char const* pathname, int flags)
    {
        return ::open(pathname, flags);
    }

    int close(int fd)
    {
        return ::close(fd);
    }

    int ioctl(int d, unsigned long int request, int val)
    {
        return ::ioctl(d, request, val);
    }

    int ioctl(int d, unsigned long int request, void* p_val)
    {
        return ::ioctl(d, request, p_val);
    }

    int tcsetattr(int d, int acts, const struct termios *tcattr)
    {
        return ::tcsetattr(d, acts, tcattr);
    }

    int tcgetattr(int d, struct termios *tcattr)
    {
        return ::tcgetattr(d, tcattr);
    }
};

struct RealPosixProcessOperations : public mir::PosixProcessOperations
{
    pid_t getpid() const override
    {
        return ::getpid();
    }
    pid_t getppid() const override
    {
        return ::getppid();
    }
    pid_t getpgid(pid_t process) const override
    {
        return ::getpgid(process);
    }
    pid_t getsid(pid_t process) const override
    {
        return ::getsid(process);
    }
    int setpgid(pid_t process, pid_t group) override
    {
        return ::setpgid(process, group);
    }
    pid_t setsid() override
    {
        return ::setsid();
    }
};
}

std::shared_ptr<mir::ConsoleServices> mir::DefaultServerConfiguration::the_console_services()
{
    auto const make_console_services =
        [this]() -> std::shared_ptr<ConsoleServices>
        {
            auto const provider = the_options()->get<std::string>(options::console_provider);
            auto const vt = the_options()->get<int>(options::vt_option_name);

            auto const logind_constructor =
                [this]()
                {
                    try
                    {
                        auto const vt_services = std::make_shared<mir::LogindConsoleServices>(
                            std::dynamic_pointer_cast<mir::GLibMainLoop>(the_main_loop()));
                        mir::log_debug("Using logind for session management");
                        return vt_services;
                    }
                    catch (std::exception const& e)
                    {
                        mir::log_debug(
                            "Not using logind for session management: %s",
                            e.what());
                        throw;
                    }
                };

            auto const linux_vt_constructor =
                [this](int vt)
                {
                    try
                    {
                        auto const vt_services = std::make_shared<mir::LinuxVirtualTerminal>(
                            std::make_unique<RealVTFileOperations>(),
                            std::make_unique<RealPosixProcessOperations>(),
                            vt,
                            *the_emergency_cleanup(),
                            the_display_report());
                        mir::log_debug("Using Linux VT subsystem for session management");
                        return vt_services;
                    }
                    catch (std::exception const& e)
                    {
                        mir::log_debug(
                            "Not using Linux VT subsystem for session management: %s",
                            e.what());
                        throw;
                    }
                };

            auto const null_constructor =
                []()
                {
                    mir::log_debug("No session management supported");
                    return std::make_shared<mir::NullConsoleServices>();
                };

            if (provider == options::auto_console)
            {
                if (vt)
                {
                    throw mir::AbnormalExit{"--vt option requires --console-provider=vt"};
                }
                try
                {
                    if (getenv("DISPLAY"))
                    {
                        mir::log_debug("Not trying logind: \"DISPLAY\" is set and X need not have claimed the VT");
                    }
                    else
                    {
                        return logind_constructor();
                    }
                }
                catch (std::exception const&)
                {
                }
                try
                {
                    return linux_vt_constructor(vt);
                }
                catch (std::exception const&)
                {
                }
                return null_constructor();
            }
            else if (provider == options::logind_console)
            {
                if (vt)
                {
                    throw mir::AbnormalExit{"--vt option requires --console-provider=vt"};
                }
                return logind_constructor();
            }
            else if (provider == options::vt_console)
            {
                return linux_vt_constructor(vt);
            }
            else if (provider == options::null_console)
            {
                if (vt)
                {
                    throw mir::AbnormalExit{"--vt option requires --console-provider=vt"};
                }
                return null_constructor();
            }

            BOOST_THROW_EXCEPTION((
                std::runtime_error{
                    std::string{"Unknown console provider: "} + provider}));
        };

    // TODO: this is clearly not threadsafe, but then neither is CachedPtr
    if (!console_services)
    {
        console_services = make_console_services();
    }

    return console_services;
}
