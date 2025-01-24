/*
 * Copyright © Canonical Ltd.
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
 */


#include "miral/x11_support.h"

#include <mir/server.h>
#include <mir/options/configuration.h>
#include <mir/main_loop.h>
#include <mir/fd.h>
#include <mir/fatal.h>

namespace mo = mir::options;

struct miral::X11Support::Self
{
    bool x11_enabled{false};
};

miral::X11Support::X11Support() : self{std::make_shared<Self>()}
{
}

void miral::X11Support::operator()(mir::Server& server) const
{
    static auto const x11_displayfd_opt = "x11-displayfd";

    server.add_configuration_option(
        mo::x11_display_opt,
        "Enable X11 support [{1|true|on|yes, 0|false|off|no}].", self->x11_enabled);

    auto const x11_scale_default = []() -> char const* {
        if (auto const gdk_scale = getenv("GDK_SCALE"))
        {
            return gdk_scale;
        }
        else
        {
            return "1.0";
        }
    }();

    server.add_configuration_option(
        mo::x11_scale_opt,
        "The scale to assume X11 apps use. Defaults to the value of GDK_SCALE or 1. Can be fractional. "
        "(Consider also setting GDK_SCALE in app-env-x11 when using this)",
        x11_scale_default);

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
                            // x11_display is in the ":<n>" format used for $DISPLAY
                            // Drop the leading ":" and append a newline...
                            auto const display = x11_display.value().substr(1) + "\n";

                            if (write(fd, display.c_str(), display.size()) != static_cast<ssize_t>(display.size()))
                            {
                                mir::fatal_error("Cannot write X11 display number to fd %d\n", static_cast<int>(fd));
                            }
                        }
                    });
            }
        });
}

miral::X11Support::~X11Support() = default;
miral::X11Support::X11Support(X11Support const&) = default;
auto miral::X11Support::operator=(X11Support const&) -> X11Support& = default;

auto miral::X11Support::default_to_enabled() -> X11Support&
{
    self->x11_enabled = true;

    return *this;
}
