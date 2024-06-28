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


#include "miral/xdg_decorations.h"

#include "mir/abnormal_exit.h"
#include "mir/misc_options.h"
#include "mir/options/configuration.h"
#include "mir/server.h"
#include "mir/xdg_decorations_interface.h"

#include <format>
#include <memory>
#include <string>

namespace mo = mir::options;

const char* xdg_decorations_always_ssd = "always-ssd";
const char* xdg_decorations_always_csd = "always-csd";
const char* xdg_decorations_prefer_ssd = "prefer-ssd";
const char* xdg_decorations_prefer_csd = "prefer-csd";

struct miral::XdgDecorations::Self
{
    Mode mode;
};

miral::XdgDecorations::XdgDecorations(std::shared_ptr<mir::options::Option> options): self{from_options(options)}
{
}

miral::XdgDecorations::XdgDecorations(Mode mode): self{std::make_shared<Self>(mode)}
{
}

void miral::XdgDecorations::operator()(mir::Server& server) const
{
    server.add_configuration_option(
        mo::xdg_decorations_opt,
        std::format(
            "Choose the preferred mode for decorations [{}, {}, {}, {}]",
            xdg_decorations_always_ssd,
            xdg_decorations_always_csd,
            xdg_decorations_prefer_ssd,
            xdg_decorations_prefer_csd),
        xdg_decorations_prefer_csd);

    server.add_pre_init_callback(
        [&]() { server.the_misc_options()->xdg_decorations = std::make_shared<XdgDecorations>(server.get_options()); });
}

auto miral::XdgDecorations::mode() const -> Mode
{
    return self->mode;
}

auto miral::XdgDecorations::from_options(std::shared_ptr<mir::options::Option> options) -> std::shared_ptr<Self> {
        const auto opt_val = options->get<std::string>(mir::options::xdg_decorations_opt);

        auto mode = Mode::prefer_csd;
        if (opt_val == xdg_decorations_always_ssd)
        {
            mode = Mode::always_ssd;
        }
        else if (opt_val == xdg_decorations_always_csd)
        {
            mode = Mode::always_csd;
        }
        else if (opt_val == xdg_decorations_prefer_ssd)
        {
            mode = Mode::prefer_ssd;
        }
        else if (opt_val == xdg_decorations_prefer_csd)
        {
            mode = Mode::prefer_csd;
        }
        else
        {
            throw mir::AbnormalExit{
                std::format("Unrecognised option for {}: {}", mir::options::xdg_decorations_opt, opt_val)};
        }

        return std::make_shared<Self>(mode);
}

miral::XdgDecorations::XdgDecorations(miral::XdgDecorations const&) = default;
auto miral::XdgDecorations::operator=(miral::XdgDecorations const&) -> XdgDecorations& = default;

auto miral::XdgDecorations::always_ssd() -> std::shared_ptr<mir::XdgDecorationsInterface>
{
    return std::make_shared<XdgDecorations>(Mode::always_ssd);
}

auto miral::XdgDecorations::always_csd() -> std::shared_ptr<mir::XdgDecorationsInterface>
{
    return std::make_shared<XdgDecorations>(Mode::always_csd);
}

auto miral::XdgDecorations::prefer_ssd() -> std::shared_ptr<mir::XdgDecorationsInterface>
{
    return std::make_shared<XdgDecorations>(Mode::prefer_ssd);
}

auto miral::XdgDecorations::prefer_csd() -> std::shared_ptr<mir::XdgDecorationsInterface>
{
    return std::make_shared<XdgDecorations>(Mode::prefer_csd);
}
