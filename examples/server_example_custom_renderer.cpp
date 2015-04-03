/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "server_example_custom_renderer.h"
#include "server_example_adorning_renderer.h"

#include "mir/abnormal_exit.h"
#include "mir/server.h"
#include "mir/options/option.h"
#include "mir/compositor/display_buffer_compositor_factory.h"

namespace me = mir::examples;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

///\example server_example_custom_renderer.cpp
/// Demonstrate writing an alternative GL rendering code

namespace
{
char const* const renderer_option = "custom-renderer";
char const* const renderer_description = "Select an alterative renderer [{adorning|default}]";
char const* const renderer_adorning = "adorning";
char const* const renderer_default = "default";

class AdorningRendererFactory : public mc::DisplayBufferCompositorFactory
{
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(
        mg::DisplayBuffer& display_buffer) override
    {
        return std::make_unique<me::AdorningRenderer>(display_buffer);
    }
};
}

void me::add_custom_renderer_option_to(Server& server)
{
    server.add_configuration_option(renderer_option, renderer_description, renderer_default);

    server.wrap_display_buffer_compositor_factory(
        [server](std::shared_ptr<mc::DisplayBufferCompositorFactory> const& factory)
        -> std::shared_ptr<mc::DisplayBufferCompositorFactory>
    {
        auto const selection = server.get_options()->get<std::string>(renderer_option);
        if (selection == renderer_adorning)
        {
            return std::make_shared<AdorningRendererFactory>(); 
        }
        else if (selection == renderer_default)
        {
            return factory;
        }

        throw mir::AbnormalExit("Unknown renderer selection: " + selection);
    });

}
