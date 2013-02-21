/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/display_server.h"
#include "mir/server_configuration.h"

#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/render_view.h"
#include "mir/sessions/session_store.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/input/input_manager.h"

#include <thread>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace mi = mir::input;

namespace
{
std::initializer_list<std::shared_ptr<mi::EventFilter> const> empty_filter_list{};
}

struct mir::DisplayServer::Private
{
    Private(ServerConfiguration& config)
        : display{config.the_display()},
          buffer_bundle_factory{
              std::make_shared<mc::BufferBundleManager>(config.the_buffer_allocation_strategy())},
          surface_stack{std::make_shared<ms::SurfaceStack>(buffer_bundle_factory.get())},
          surface_factory{std::make_shared<ms::SurfaceController>(surface_stack.get())},
          compositor{std::make_shared<mc::Compositor>(surface_stack.get(), config.the_renderer())},
          session_store{config.the_session_store(surface_factory, display)},
          communicator{config.the_communicator(session_store, config.the_buffer_allocator())},
          input_manager{config.the_input_manager(empty_filter_list, display)},
          exit(false)
    {
    }

    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mc::BufferBundleFactory> buffer_bundle_factory;
    std::shared_ptr<ms::SurfaceStack> surface_stack;
    std::shared_ptr<sessions::SurfaceFactory> surface_factory;
    std::shared_ptr<mc::Compositor> compositor;
    std::shared_ptr<sessions::SessionStore> session_store;
    std::shared_ptr<frontend::Communicator> communicator;
    std::shared_ptr<mi::InputManager> input_manager;
    std::mutex exit_guard;
    bool exit;
};

mir::DisplayServer::DisplayServer(ServerConfiguration& config) :
    p()
{
    p.reset(new mir::DisplayServer::Private(config));
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
