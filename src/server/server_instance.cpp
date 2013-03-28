/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/server_instance.h"
#include "mir/server_configuration.h"

#include "mir/compositor/compositor.h"
#include "mir/frontend/shell.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/display.h"
#include "mir/input/input_manager.h"

#include <mutex>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;

struct mir::ServerInstance::Private
{
    Private(ServerConfiguration& config)
        : display{config.the_display()},
          graphics_platform{config.the_graphics_platform()},
          compositor{config.the_compositor()},
          shell{config.the_frontend_shell()},
          surface_factory{config.the_surface_factory()},
          communicator{config.the_communicator()},
          input_manager{config.the_input_manager()},
          ready_to_run{config.the_ready_to_run_handler()},
          exit(false)
    {
    }

    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mg::Platform> graphics_platform;
    std::shared_ptr<mc::Compositor> compositor;
    std::shared_ptr<frontend::Shell> shell;    
    std::shared_ptr<msh::SurfaceFactory> surface_factory;
    std::shared_ptr<mf::Communicator> communicator;
    std::shared_ptr<mi::InputManager> input_manager;
    std::function<void(mir::DisplayServer*)> ready_to_run;
    std::mutex exit_guard;
    std::condition_variable exit_cv;
    bool exit;
};

mir::ServerInstance::ServerInstance(ServerConfiguration& config) :
    p()
{
    p.reset(new ServerInstance::Private(config));
}

mir::ServerInstance::~ServerInstance()
{
    p->shell->shutdown();
}

void mir::ServerInstance::run()
{
    std::unique_lock<std::mutex> lk(p->exit_guard);

    p->communicator->start();
    p->compositor->start();
    p->input_manager->start();

    if (p->ready_to_run)
        p->ready_to_run(this);
    
    while (!p->exit)
        p->exit_cv.wait(lk);

    p->input_manager->stop();
    p->compositor->stop();
}

void mir::ServerInstance::stop()
{
    std::unique_lock<std::mutex> lk(p->exit_guard);
    p->exit = true;
    p->exit_cv.notify_one();
}

std::shared_ptr<mg::Platform> mir::ServerInstance::graphics_platform()
{
    return p->graphics_platform;
}

std::shared_ptr<msh::SurfaceFactory> mir::ServerInstance::surface_factory()
{
    return p->surface_factory;
}
