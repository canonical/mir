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

#include "miral/custom_decorations.h"
#include "decoration_manager_adapter.h"

#include "miral/decoration_manager_builder.h"

#include "mir/options/option.h"
#include "mir/server.h"
#define MIR_LOG_COMPONENT "CustomDecorations"
#include "mir/log.h"

#include <memory>


struct miral::CustomDecorations::Self
{
    std::shared_ptr<miral::DecorationManagerAdapter> adapter;
};

miral::CustomDecorations::CustomDecorations(std::shared_ptr<miral::DecorationManagerAdapter> manager_adapter) :
    self{std::make_unique<Self>(manager_adapter)}
{
}

void miral::CustomDecorations::operator()(mir::Server& server) const
{
    static auto const custom_light_decorations_opt = "enable-custom-decorations";
    auto should_use_light_decorations = false;

    server.add_configuration_option(
        custom_light_decorations_opt,
        "Enable custom themed decorations [{1|true|on|yes, 0|false|off|no}].",
        should_use_light_decorations);

    server.add_pre_init_callback(
        [&]
        {
            auto const options = server.get_options();
            if (options->is_set(custom_light_decorations_opt))
            {
                mir::log_info("Using custom decorations!");
                server.set_the_decoration_manager_init(
                    [adapter = self->adapter]
                    {
                        return adapter;
                    });
            }
        });
}
