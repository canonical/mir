/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/display_server.h"
#include "mir/server_configuration.h"
#include "mir/main_loop.h"

#include "mir/compositor/compositor.h"
#include "mir/frontend/shell.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/display.h"
#include "mir/input/input_manager.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;

struct mir::DisplayServer::Private
{
    Private(ServerConfiguration& config)
        : display{config.the_display()},
          compositor{config.the_compositor()},
          shell{config.the_frontend_shell()},
          communicator{config.the_communicator()},
          input_manager{config.the_input_manager()},
          main_loop{config.the_main_loop()}
    {
        display->register_pause_resume_handlers(
            *main_loop,
            [this]
            {
                compositor->stop();
                display->pause();
            },
            [this]
            {
                display->resume();
                compositor->start();
            });
    }

    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mc::Compositor> compositor;
    std::shared_ptr<frontend::Shell> shell;
    std::shared_ptr<mf::Communicator> communicator;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<mir::MainLoop> main_loop;
};

mir::DisplayServer::DisplayServer(ServerConfiguration& config) :
    p()
{
    p.reset(new DisplayServer::Private(config));
}

mir::DisplayServer::~DisplayServer()
{
    p->shell->shutdown();
}

void mir::DisplayServer::run()
{
    p->communicator->start();
    p->compositor->start();
    p->input_manager->start();

    p->main_loop->run();

    p->input_manager->stop();
    p->compositor->stop();
}

void mir::DisplayServer::stop()
{
    p->main_loop->stop();
}
