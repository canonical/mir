/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "inprocess_egl_client.h"

#include "mir/run_mir.h"
#include "mir/default_server_configuration.h"

namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace me = mir::examples;

namespace
{

struct InprocessClientStarter
{
    InprocessClientStarter(std::shared_ptr<msh::SurfaceFactory> const& surface_factory,
                           std::shared_ptr<mg::Platform> const& graphics_platform)
      : surface_factory(surface_factory),
        graphics_platform(graphics_platform)
    {
    }

    void operator()(mir::DisplayServer&)
    {
        auto client = new me::InprocessEGLClient(graphics_platform, surface_factory);
    }
    
    std::shared_ptr<msh::SurfaceFactory> const surface_factory;
    std::shared_ptr<mg::Platform> const graphics_platform;
};

}


int main(int argc, char const* argv[])
{
    mir::DefaultServerConfiguration config(argc, argv);
    
    mir::run_mir(config, InprocessClientStarter(config.the_surface_factory(), config.the_graphics_platform()));
}
