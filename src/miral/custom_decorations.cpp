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

#include "mir/shell/decoration/manager.h"
#include "miral/decoration_manager_builder.h"

#include "mir/options/option.h"
#include "mir/log.h"
#include "mir/server.h"

#include <memory>


struct miral::CustomDecorations::Self
{
    ManagerAdapterBuilder adapter;
};

miral::CustomDecorations::CustomDecorations(miral::CustomDecorations::ManagerAdapterBuilder manager_adapter_builder) :
    self{std::make_unique<Self>(manager_adapter_builder)}
{
}

void miral::CustomDecorations::operator()(mir::Server& server) const
{
    static auto const custom_light_decorations_opt = "enable-custom-decorations";
    auto should_use_light_decorations = true;

    server.add_configuration_option(
        custom_light_decorations_opt,
        "Enable custom themed decorations [{1|true|on|yes, 0|false|off|no}].",
        should_use_light_decorations);

    server.wrap_decoration_manager(
        [this, &server](auto deco_man) -> std::shared_ptr<mir::shell::decoration::Manager>
        {
            auto const& options = server.get_options();
            if (!options->is_set(custom_light_decorations_opt) || !options->get<bool>(custom_light_decorations_opt))
            {
                mir::log_info("Using default decorations!");
                return deco_man;
            }

            mir::log_info("Using custom decorations!");
            return self->adapter(server);
        });
}
