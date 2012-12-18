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
#include "mir/frontend/session_manager.h"
#include "mir/frontend/communicator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/input/input_manager.h"

#include "mir/thread/all.h"

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
        : graphics_platform{config.make_graphics_platform()},
          display{graphics_platform->create_display()},
          buffer_allocator{graphics_platform->create_buffer_allocator(config.make_buffer_initializer())},
          buffer_allocation_strategy{config.make_buffer_allocation_strategy(buffer_allocator)},
          buffer_bundle_manager{std::make_shared<mc::BufferBundleManager>(buffer_allocation_strategy)},
          surface_stack{std::make_shared<ms::SurfaceStack>(buffer_bundle_manager.get())},
          surface_controller{std::make_shared<ms::SurfaceController>(surface_stack.get())},
          renderer{config.make_renderer(display)},
          compositor{std::make_shared<mc::Compositor>(surface_stack.get(), renderer)},
          application_session_factory{config.make_session_manager(surface_controller)},
          communicator{config.make_communicator(application_session_factory, display, buffer_allocator)},
          input_manager{config.make_input_manager(empty_filter_list, display)},
          exit(false)
    {
    }

    std::shared_ptr<mg::Platform> graphics_platform;
    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mc::GraphicBufferAllocator> buffer_allocator;
    std::shared_ptr<mc::BufferAllocationStrategy> buffer_allocation_strategy;
    std::shared_ptr<mc::BufferBundleManager> buffer_bundle_manager;
    std::shared_ptr<ms::SurfaceStack> surface_stack;
    std::shared_ptr<ms::SurfaceController> surface_controller;
    std::shared_ptr<mg::Renderer> renderer;
    std::shared_ptr<mc::Compositor> compositor;
    std::shared_ptr<frontend::SessionManager> application_session_factory;
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
    p->application_session_factory->shutdown();
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
    display->clear();
    p->compositor->render(display);
}
