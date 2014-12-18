/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_window_manager.h"

#include "mir/server.h"
#include "mir/options/option.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/surface_creation_parameters.h"

namespace me = mir::examples;
namespace ms = mir::scene;
namespace msh = mir::shell;

///\example server_example_window_manager.cpp
/// Demonstrate a simple window management strategy

namespace
{
class WindowManager : public ms::PlacementStrategy
{
    auto place(ms::Session const&, ms::SurfaceCreationParameters const& request_parameters)
    -> ms::SurfaceCreationParameters
    {
        return request_parameters;
    }

};

class WindowManagmentFactory
{
public:
    auto make() -> std::shared_ptr<WindowManager>
    {
        auto tmp = wm.lock();

        if (!tmp)
        {
            tmp = std::make_shared<WindowManager>();
            wm = tmp;
        }

        return tmp;
    }

private:
    std::weak_ptr<WindowManager> wm;
};

}

void me::add_window_manager_option_to(Server& server)
{
    static auto const option = "window-manager";
    static auto const description = "window management strategy [{tiling}]";
    server.add_configuration_option(option, description, mir::OptionType::string);

    auto const factory = std::make_shared<WindowManagmentFactory>();

    server.override_the_placement_strategy([factory, &server]()
        -> std::shared_ptr<ms::PlacementStrategy>
        {
            auto const options = server.get_options();
            if (options->is_set(option) && options->get<std::string>(option) == "tiling")
                return factory->make();
            else
                return std::shared_ptr<ms::PlacementStrategy>{};
        });
}
