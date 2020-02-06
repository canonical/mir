/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#include "miral/x11_support.h"

#include <mir/server.h>
#include <mir/options/configuration.h>
#include <mir/main_loop.h>
#include <mir/fd.h>

namespace mo = mir::options;

struct miral::X11Support::Self
{
};

miral::X11Support::X11Support() : self{std::make_shared<Self>()}
{
}

void miral::X11Support::operator()(mir::Server& server) const
{
    static auto const x11_displayfd_opt = "x11-displayfd";

    server.add_configuration_option(
        mo::x11_display_opt,
        "Enable X11 support", mir::OptionType::null);

    server.add_configuration_option(
        "xwayland-path",
        "Path to Xwayland executable", "/usr/bin/Xwayland");

    server.add_configuration_option(
        x11_displayfd_opt,
        "file descriptor to write X11 DISPLAY number to when ready to connect", mir::OptionType::integer);

    server.add_init_callback([this, &server]
        {
            auto const options = server.get_options();

            if (options->is_set(x11_displayfd_opt))
            {
                mir::Fd const fd{options->get<int>(x11_displayfd_opt)};

                server.the_main_loop()->enqueue(this, [fd, &server]
                    {
                        if (auto const x11_display = server.x11_display())
                        {
                            auto const display = x11_display.value().substr(1) + "\n";

                            if (write(fd, display.c_str(), display.size()) != static_cast<ssize_t>(display.size()))
                            {
                                mir::fatal_error("Cannot write X11 display number to fd %d\n", fd);
                            }
                        }
                    });
            }
        });
}

miral::X11Support::~X11Support() = default;
miral::X11Support::X11Support(X11Support const&) = default;
auto miral::X11Support::operator=(X11Support const&) -> X11Support& = default;


