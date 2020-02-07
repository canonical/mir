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

namespace mo = mir::options;

struct miral::X11Support::Self
{
};

miral::X11Support::X11Support() : self{std::make_shared<Self>()}
{
}

void miral::X11Support::operator()(mir::Server& server) const
{
    server.add_configuration_option(
        mo::x11_display_opt,
        "Enable X11 support", mir::OptionType::null);

    server.add_configuration_option(
        "xwayland-path",
        "Path to Xwayland executable", "/usr/bin/Xwayland");
}

miral::X11Support::~X11Support() = default;
miral::X11Support::X11Support(X11Support const&) = default;
auto miral::X11Support::operator=(X11Support const&) -> X11Support& = default;


