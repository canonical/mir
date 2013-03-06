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

#include "mir/display_server.h"
#include "mir/server_configuration.h"

#include "mir/compositor/drawer.h"
#include "mir/shell/session_store.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/display.h"
#include "mir/input/input_manager.h"

#include <mutex>
#include <thread>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;

struct mir::DisplayServer::Private
{
    Private(ServerConfiguration& config)
        : display{config.the_display()},
          compositor{config.the_drawer()},
          session_store{config.the_session_store()},
          communicator{config.the_communicator()},
          input_manager{config.the_input_manager()},
          exit(false)
    {
    }

    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mc::Drawer> compositor;
    std::shared_ptr<shell::SessionStore> session_store;
    std::shared_ptr<mf::Communicator> communicator;
    std::shared_ptr<mi::InputManager> input_manager;
    std::mutex exit_guard;
    bool exit;
};

mir::DisplayServer::DisplayServer(ServerConfiguration& config) :
    p()
{
    p.reset(new DisplayServer::Private(config));
}

mir::DisplayServer::~DisplayServer()
{
    p->session_store->shutdown();
}

void mir::DisplayServer::start()
{
    p->communicator->start();
    p->input_manager->start();

    std::unique_lock<std::mutex> lk(p->exit_guard);
    while (!p->exit)
    {
        lk.unlock();
        do_stuff();
        lk.lock();
    }

    p->input_manager->stop();
}

void mir::DisplayServer::do_stuff()
{
    render(p->display.get());
}

void mir::DisplayServer::stop()
{
    std::unique_lock<std::mutex> lk(p->exit_guard);
    p->exit=true;
}

void mir::DisplayServer::render(mg::Display* display)
{
    p->compositor->render(display);
}
