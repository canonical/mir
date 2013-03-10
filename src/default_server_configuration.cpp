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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"

#include "mir/options/program_option.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/application_mediator_report.h"
#include "mir/frontend/application_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/shell/session_manager.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/single_visibility_focus_mechanism.h"
#include "mir/shell/session_container.h"
#include "mir/shell/consuming_placement_strategy.h"
#include "mir/shell/organising_surface_factory.h"
#include "mir/graphics/display.h"
#include "mir/graphics/gl_renderer.h"
#include "mir/graphics/renderer.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/null_display_report.h"
#include "mir/input/input_manager.h"
#include "mir/logging/logger.h"
#include "mir/logging/dumb_console_logger.h"
#include "mir/logging/application_mediator_report.h"
#include "mir/logging/display_report.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace ms = mir::surfaces;
namespace msh = mir::shell;
namespace mi = mir::input;

namespace
{
std::initializer_list<std::shared_ptr<mi::EventFilter> const> empty_filter_list{};
}

namespace
{
class DefaultIpcFactory : public mf::ProtobufIpcFactory
{
public:
    explicit DefaultIpcFactory(
        std::shared_ptr<msh::SessionStore> const& session_store,
        std::shared_ptr<mf::ApplicationMediatorReport> const& report,
        std::shared_ptr<mg::Platform> const& graphics_platform,
        std::shared_ptr<mg::ViewableArea> const& graphics_display,
        std::shared_ptr<mc::GraphicBufferAllocator> const& buffer_allocator) :
        session_store(session_store),
        report(report),
        cache(std::make_shared<mf::ResourceCache>()),
        graphics_platform(graphics_platform),
        graphics_display(graphics_display),
        buffer_allocator(buffer_allocator)
    {
    }

private:
    std::shared_ptr<msh::SessionStore> session_store;
    std::shared_ptr<mf::ApplicationMediatorReport> const report;
    std::shared_ptr<mf::ResourceCache> const cache;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<mg::ViewableArea> const graphics_display;
    std::shared_ptr<mc::GraphicBufferAllocator> const buffer_allocator;

    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server()
    {
        return std::make_shared<mf::ApplicationMediator>(
            session_store,
            graphics_platform,
            graphics_display,
            buffer_allocator,
            report,
            resource_cache());
    }

    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }
};

char const* const log_app_mediator = "log-app-mediator";
char const* const log_display      = "log-display";

boost::program_options::options_description program_options()
{
    namespace po = boost::program_options;

    po::options_description desc(
        "Command-line options.\n"
        "Environment variables capitalise long form with prefix \"MIR_SERVER_\" and \"_\" in place of \"-\"");
    desc.add_options()
        ("file,f", po::value<std::string>(), "socket filename")
        ("ipc-thread-pool,i", po::value<int>(), "threads in frontend thread pool")
        (log_display, po::value<bool>(), "log the Display report")
        (log_app_mediator, po::value<bool>(), "log the ApplicationMediator report")
        ("tests-use-real-graphics", po::value<bool>(), "use real graphics in tests")
        ("tests-use-real-input", po::value<bool>(), "use real input in tests");

    return desc;
}


void parse_arguments(
    std::shared_ptr<mir::options::ProgramOption> const& options,
    int argc,
    char const* argv[])
{
    namespace po = boost::program_options;

    auto desc = program_options();

    try
    {
        desc.add_options()
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
    auto desc = program_options();

    options->parse_environment(desc, "MIR_SERVER_");
}
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

std::shared_ptr<mg::DisplayReport> mir::DefaultServerConfiguration::the_display_report()
{
    return display_report(
        [this]() -> std::shared_ptr<graphics::DisplayReport>
        {
            if (the_options()->get(log_display, false))
            {
                return std::make_shared<ml::DisplayReport>(the_logger());
            }
            else
            {
                return std::make_shared<mg::NullDisplayReport>();
            }
        });
}

std::shared_ptr<mg::Platform> mir::DefaultServerConfiguration::the_graphics_platform()
{
    return graphics_platform(
        [this]()
        {
            // TODO I doubt we need the extra level of indirection provided by
            // mg::create_platform() - we just need to move the implementation
            // of DefaultServerConfiguration::the_graphics_platform() to the
            // graphics libraries.
            // Alternatively, if we want to dynamically load the graphics library
            // then this would be the place to do that.
            return mg::create_platform(the_display_report());
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
mir::DefaultServerConfiguration::the_buffer_allocation_strategy()
{
    return buffer_allocation_strategy(
        [this]()
        {
             return std::make_shared<mc::SwapperFactory>(the_buffer_allocator());
        });
}

std::shared_ptr<mg::Renderer> mir::DefaultServerConfiguration::the_renderer()
{
    return renderer(
        [&]()
        {
             return std::make_shared<mg::GLRenderer>(the_display()->view_area().size);
        });
}

std::shared_ptr<msh::SessionStore>
mir::DefaultServerConfiguration::the_session_store()
{
    return session_store(
        [this]() -> std::shared_ptr<msh::SessionStore>
        {
            auto session_container = std::make_shared<msh::SessionContainer>();
            auto focus_mechanism = std::make_shared<msh::SingleVisibilityFocusMechanism>(session_container);
            auto focus_selection_strategy = std::make_shared<msh::RegistrationOrderFocusSequence>(session_container);

            auto placement_strategy = std::make_shared<msh::ConsumingPlacementStrategy>(the_display());
            auto organising_factory = std::make_shared<msh::OrganisingSurfaceFactory>(the_surface_factory(), placement_strategy);

            return std::make_shared<msh::SessionManager>(organising_factory, session_container, focus_selection_strategy, focus_mechanism);
        });
}

std::initializer_list<std::shared_ptr<mi::EventFilter> const>
mir::DefaultServerConfiguration::the_event_filters()
{
    return empty_filter_list;
}

std::shared_ptr<mi::InputManager>
mir::DefaultServerConfiguration::the_input_manager()
{
    return input_manager(
        [&, this]()
        {
            return mi::create_input_manager(the_event_filters(), the_display());
        });
}

std::shared_ptr<mc::GraphicBufferAllocator>
mir::DefaultServerConfiguration::the_buffer_allocator()
{
    return buffer_allocator(
        [&]()
        {
            return the_graphics_platform()->create_buffer_allocator(the_buffer_initializer());
        });
}

std::shared_ptr<mg::Display>
mir::DefaultServerConfiguration::the_display()
{
    return display(
        [this]()
        {
            return the_graphics_platform()->create_display();
        });
}

std::shared_ptr<mg::ViewableArea> mir::DefaultServerConfiguration::the_viewable_area()
{
    return the_display();
}

std::shared_ptr<ms::SurfaceStackModel>
mir::DefaultServerConfiguration::the_surface_stack_model()
{
    return surface_stack(
        [this]()
        {
            return std::make_shared<ms::SurfaceStack>(the_buffer_bundle_factory());
        });
}

std::shared_ptr<mc::RenderView>
mir::DefaultServerConfiguration::the_render_view()
{
    return surface_stack(
        [this]()
        {
            return std::make_shared<ms::SurfaceStack>(the_buffer_bundle_factory());
        });
}

std::shared_ptr<msh::SurfaceFactory>
mir::DefaultServerConfiguration::the_surface_factory()
{
    return surface_controller(
        [this]()
        {
            return std::make_shared<ms::SurfaceController>(the_surface_stack_model());
        });
}

std::shared_ptr<mc::Drawer>
mir::DefaultServerConfiguration::the_drawer()
{
    return compositor(
        [this]()
        {
            return std::make_shared<mc::Compositor>(the_render_view(), the_renderer());
        });
}

std::shared_ptr<ms::BufferBundleFactory>
mir::DefaultServerConfiguration::the_buffer_bundle_factory()
{
    return buffer_bundle_manager(
        [this]()
        {
            return std::make_shared<mc::BufferBundleManager>(the_buffer_allocation_strategy());
        });
}


std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::the_ipc_factory(
    std::shared_ptr<msh::SessionStore> const& session_store,
    std::shared_ptr<mg::ViewableArea> const& display,
    std::shared_ptr<mc::GraphicBufferAllocator> const& allocator)
{
    return ipc_factory(
        [&]()
        {
            return std::make_shared<DefaultIpcFactory>(
                session_store,
                the_application_mediator_report(),
                the_graphics_platform(),
                display, allocator);
        });
}

std::shared_ptr<mf::ApplicationMediatorReport>
mir::DefaultServerConfiguration::the_application_mediator_report()
{
    return application_mediator_report(
        [this]() -> std::shared_ptr<mf::ApplicationMediatorReport>
        {
            if (the_options()->get(log_app_mediator, false))
            {
                return std::make_shared<ml::ApplicationMediatorReport>(the_logger());
            }
            else
            {
                return std::make_shared<mf::NullApplicationMediatorReport>();
            }
        });
}

std::shared_ptr<ml::Logger> mir::DefaultServerConfiguration::the_logger()
{
    return logger(
        [this]()
        {
            // TODO use the_options() to configure logging
            return std::make_shared<ml::DumbConsoleLogger>();
        });
}
