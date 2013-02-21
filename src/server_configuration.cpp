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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/server_configuration.h"

#include "mir/options/program_option.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/application_listener.h"
#include "mir/frontend/application_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/sessions/session_manager.h"
#include "mir/sessions/registration_order_focus_sequence.h"
#include "mir/sessions/single_visibility_focus_mechanism.h"
#include "mir/sessions/session_container.h"
#include "mir/sessions/consuming_placement_strategy.h"
#include "mir/sessions/organising_surface_factory.h"
#include "mir/graphics/display.h"
#include "mir/graphics/gl_renderer.h"
#include "mir/graphics/renderer.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/input/input_manager.h"
#include "mir/logging/logger.h"
#include "mir/logging/dumb_console_logger.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace ms = mir::surfaces;
namespace msess = mir::sessions;
namespace mi = mir::input;

namespace
{
class DefaultIpcFactory : public mf::ProtobufIpcFactory
{
public:
    explicit DefaultIpcFactory(
        std::shared_ptr<msess::SessionStore> const& session_store,
        std::shared_ptr<mf::ApplicationListener> const& listener,
        std::shared_ptr<mg::Platform> const& graphics_platform,
        std::shared_ptr<mg::Display> const& graphics_display,
        std::shared_ptr<mc::GraphicBufferAllocator> const& buffer_allocator) :
        session_store(session_store),
        listener(listener),
        cache(std::make_shared<mf::ResourceCache>()),
        graphics_platform(graphics_platform),
        graphics_display(graphics_display),
        buffer_allocator(buffer_allocator)
    {
    }

private:
    std::shared_ptr<msess::SessionStore> session_store;
    std::shared_ptr<mf::ApplicationListener> const listener;
    std::shared_ptr<mf::ResourceCache> const cache;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<mg::Display> const graphics_display;
    std::shared_ptr<mc::GraphicBufferAllocator> const buffer_allocator;

    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server()
    {
        return std::make_shared<mf::ApplicationMediator>(
            session_store,
            graphics_platform,
            graphics_display,
            buffer_allocator,
            listener,
            resource_cache());
    }

    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }
};

void parse_arguments(
    std::shared_ptr<mir::options::ProgramOption> const& options,
    int argc,
    char const* argv[])
{
    namespace po = boost::program_options;

    po::options_description desc("Options");

    try
    {
        namespace po = boost::program_options;

        desc.add_options()
            ("file,f", po::value<std::string>(), "<socket filename>")
            ("help,h", "this help text");

        options->parse_arguments(desc, argc, argv);

        if (options->is_set("help"))
        {
            BOOST_THROW_EXCEPTION(po::error("help text requested"));
        }
    }
    catch (po::error const& error)
    {
        std::cerr << desc << "\n";
        throw;
    }
}

void parse_environment(std::shared_ptr<mir::options::ProgramOption> const& options)
{
    namespace po = boost::program_options;

    po::options_description desc("Environment options");
    desc.add_options()
        ("tests_use_real_graphics", po::value<bool>(), "use real graphics in tests")
        ("ipc_thread_pool", po::value<int>(), "threads in frontend thread pool")
        ("tests_use_real_input", po::value<bool>(), "use real input in tests");

    options->parse_environment(desc, "MIR_SERVER_");
}
}

mir::DefaultServerConfiguration::DefaultServerConfiguration()
{
    auto options = std::make_shared<mir::options::ProgramOption>();

    parse_environment(options);

    this->options = options;
}

mir::DefaultServerConfiguration::DefaultServerConfiguration(int argc, char const* argv[])
{
    auto options = std::make_shared<mir::options::ProgramOption>();

    parse_arguments(options, argc, argv);
    parse_environment(options);

    this->options = options;
}

std::string mir::DefaultServerConfiguration::the_socket_file() const
{
    return the_options()->get("file", "/tmp/mir_socket");
}

std::shared_ptr<mir::options::Option> mir::DefaultServerConfiguration::the_options() const
{
    return options;
}

std::shared_ptr<mg::Platform> mir::DefaultServerConfiguration::the_graphics_platform()
{
    return graphics_platform(
        []()
        {
            // TODO I doubt we need the extra level of indirection provided by
            // mg::create_platform() - we just need to move the implementation
            // of DefaultServerConfiguration::the_graphics_platform() to the
            // graphics libraries.
            // Alternatively, if we want to dynamically load the graphics library
            // then this would be the place to do that.
             return mg::create_platform();
        });
}

std::shared_ptr<mg::BufferInitializer>
mir::DefaultServerConfiguration::the_buffer_initializer()
{
    return buffer_initializer(
        []()
        {
             return std::make_shared<mg::NullBufferInitializer>();
        });
}

std::shared_ptr<mc::BufferAllocationStrategy>
mir::DefaultServerConfiguration::the_buffer_allocation_strategy(
        std::shared_ptr<mc::GraphicBufferAllocator> const& buffer_allocator)
{
    return buffer_allocation_strategy(
        [&]()
        {
             return std::make_shared<mc::SwapperFactory>(buffer_allocator);
        });
}

std::shared_ptr<mg::Renderer> mir::DefaultServerConfiguration::the_renderer(
        std::shared_ptr<mg::Display> const& display)
{
    return renderer(
        [&]()
        {
             return std::make_shared<mg::GLRenderer>(display->view_area().size);
        });
}

std::shared_ptr<msess::SessionStore>
mir::DefaultServerConfiguration::the_session_store(
    std::shared_ptr<msess::SurfaceFactory> const& surface_factory,
    std::shared_ptr<mg::ViewableArea> const& viewable_area)
{
    return session_store(
        [&,this]() -> std::shared_ptr<msess::SessionStore>
        {
            auto session_container = std::make_shared<msess::SessionContainer>();
            auto focus_mechanism = std::make_shared<msess::SingleVisibilityFocusMechanism>(session_container);
            auto focus_selection_strategy = std::make_shared<msess::RegistrationOrderFocusSequence>(session_container);

            auto placement_strategy = std::make_shared<msess::ConsumingPlacementStrategy>(viewable_area);
            auto organising_factory = std::make_shared<msess::OrganisingSurfaceFactory>(surface_factory, placement_strategy);

            return std::make_shared<msess::SessionManager>(organising_factory, session_container, focus_selection_strategy, focus_mechanism);
        });
}

std::shared_ptr<mi::InputManager>
mir::DefaultServerConfiguration::the_input_manager(
    const std::initializer_list<std::shared_ptr<mi::EventFilter> const>& event_filters,
    std::shared_ptr<mg::ViewableArea> const& view_area)
{
    return input_manager(
        [&]()
        {
            return mi::create_input_manager(event_filters, view_area);
        });
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::the_ipc_factory(
    std::shared_ptr<msess::SessionStore> const& session_store,
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::GraphicBufferAllocator> const& allocator)
{
    return ipc_factory(
        [&]()
        {
            return std::make_shared<DefaultIpcFactory>(
                session_store,
                the_application_listener(),
                the_graphics_platform(),
                display, allocator);
        });
}

std::shared_ptr<mf::ApplicationListener>
mir::DefaultServerConfiguration::the_application_listener()
{
    return application_listener(
        []()
        {
            return std::make_shared<mf::NullApplicationListener>();
        });
}
