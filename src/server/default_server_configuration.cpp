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
#include "mir/abnormal_exit.h"
#include "mir/asio_main_loop.h"

#include "mir/options/program_option.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/default_compositing_strategy.h"
#include "mir/compositor/multi_threaded_compositor.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/null_message_processor_report.h"
#include "mir/frontend/session_mediator.h"
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
#include "mir/input/null_input_manager.h"
#include "mir/logging/logger.h"
#include "mir/logging/dumb_console_logger.h"
#include "mir/logging/glog_logger.h"
#include "mir/logging/session_mediator_report.h"
#include "mir/logging/message_processor_report.h"
#include "mir/logging/display_report.h"
#include "mir/shell/surface_source.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/time/high_resolution_clock.h"

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
        std::shared_ptr<mf::Shell> const& shell,
        std::shared_ptr<mf::SessionMediatorReport> const& sm_report,
        std::shared_ptr<mf::MessageProcessorReport> const& mr_report,
        std::shared_ptr<mg::Platform> const& graphics_platform,
        std::shared_ptr<mg::ViewableArea> const& graphics_display,
        std::shared_ptr<mc::GraphicBufferAllocator> const& buffer_allocator) :
        shell(shell),
        sm_report(sm_report),
        mp_report(mr_report),
        cache(std::make_shared<mf::ResourceCache>()),
        graphics_platform(graphics_platform),
        graphics_display(graphics_display),
        buffer_allocator(buffer_allocator)
    {
    }

private:
    std::shared_ptr<mf::Shell> shell;
    std::shared_ptr<mf::SessionMediatorReport> const sm_report;
    std::shared_ptr<mf::MessageProcessorReport> const mp_report;
    std::shared_ptr<mf::ResourceCache> const cache;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<mg::ViewableArea> const graphics_display;
    std::shared_ptr<mc::GraphicBufferAllocator> const buffer_allocator;

    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server()
    {
        return std::make_shared<mf::SessionMediator>(
            shell,
            graphics_platform,
            graphics_display,
            buffer_allocator,
            sm_report,
            resource_cache());
    }

    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }

    virtual std::shared_ptr<mf::MessageProcessorReport> report()
    {
        return mp_report;
    }
};

char const* const log_app_mediator = "log-app-mediator";
char const* const log_msg_processor = "log-msg-processor";
char const* const log_display      = "log-display";

char const* const glog                 = "glog";
char const* const glog_stderrthreshold = "glog-stderrthreshold";
char const* const glog_minloglevel     = "glog-minloglevel";
char const* const glog_log_dir         = "glog-log-dir";

boost::program_options::options_description program_options()
{
    namespace po = boost::program_options;

    po::options_description desc(
        "Command-line options.\n"
        "Environment variables capitalise long form with prefix \"MIR_SERVER_\" and \"_\" in place of \"-\"");
    desc.add_options()
        ("file,f", po::value<std::string>(),    "Socket filename")
        ("enable-input,i", po::value<bool>(),   "Enable input. [bool:default=false]")
        (log_display, po::value<bool>(),        "Log the Display report. [bool:default=false]")
        (log_app_mediator, po::value<bool>(),   "Log the ApplicationMediator report. [bool:default=false]")
        (log_msg_processor, po::value<bool>(), "log the MessageProcessor report")
        (glog,                                  "Use google::GLog for logging")
        (glog_stderrthreshold, po::value<int>(),"Copy log messages at or above this level "
                                                "to stderr in addition to logfiles. The numbers "
                                                "of severity levels INFO, WARNING, ERROR, and "
                                                "FATAL are 0, 1, 2, and 3, respectively."
                                                " [int:default=2]")
        (glog_minloglevel, po::value<int>(),    "Log messages at or above this level. The numbers "
                                                "of severity levels INFO, WARNING, ERROR, and "
                                                "FATAL are 0, 1, 2, and 3, respectively."
                                                " [int:default=0]")
        (glog_log_dir, po::value<std::string>(),"If specified, logfiles are written into this "
                                                "directory instead of the default logging directory."
                                                " [string:default=\"\"]")
        ("ipc-thread-pool,i", po::value<int>(), "threads in frontend thread pool. [int:default=10]")
        ("tests-use-real-graphics", po::value<bool>(), "Use real graphics in tests. [bool:default=false]")
        ("tests-use-real-input", po::value<bool>(), "Use real input in tests. [bool:default=false]");

    return desc;
}


void parse_arguments(
    std::shared_ptr<mir::options::ProgramOption> const& options,
    int argc,
    char const* argv[])
{
    namespace po = boost::program_options;

    bool exit_with_helptext = false;

    auto desc = program_options();

    try
    {
        desc.add_options()
            ("help,h", "this help text");

        options->parse_arguments(desc, argc, argv);

        if (options->is_set("help"))
        {
            exit_with_helptext = true;
        }
    }
    catch (po::error const& error)
    {
        exit_with_helptext = true;
    }

    if (exit_with_helptext)
    {
        std::ostringstream help_text;
        help_text << desc;

        BOOST_THROW_EXCEPTION(mir::AbnormalExit(help_text.str()));
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

std::shared_ptr<mf::Shell>
mir::DefaultServerConfiguration::the_frontend_shell()
{
    return session_manager(
        [this]() -> std::shared_ptr<msh::SessionManager>
        {
            auto session_container = std::make_shared<msh::SessionContainer>();
            auto focus_mechanism = std::make_shared<msh::SingleVisibilityFocusMechanism>(session_container, the_input_focus_selector());
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
        [&, this]() -> std::shared_ptr<mi::InputManager>
        {
            if (the_options()->get("enable-input", false))
                return mi::create_input_manager(the_event_filters(), the_display());
            else 
                return std::make_shared<mi::NullInputManager>();
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

std::shared_ptr<mc::Renderables>
mir::DefaultServerConfiguration::the_renderables()
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
    return surface_source(
        [this]()
        {
            return std::make_shared<msh::SurfaceSource>(the_surface_builder(), the_input_channel_factory());
        });
}

std::shared_ptr<msh::SurfaceBuilder>
mir::DefaultServerConfiguration::the_surface_builder()
{
    return surface_controller(
        [this]()
        {
            return std::make_shared<ms::SurfaceController>(the_surface_stack_model());
        });
}

std::shared_ptr<mc::CompositingStrategy>
mir::DefaultServerConfiguration::the_compositing_strategy()
{
    return compositing_strategy(
        [this]()
        {
            return std::make_shared<mc::DefaultCompositingStrategy>(the_renderables(), the_renderer());
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

std::shared_ptr<mc::Compositor>
mir::DefaultServerConfiguration::the_compositor()
{
    return compositor(
        [this]()
        {
            return std::make_shared<mc::MultiThreadedCompositor>(the_display(),
                                                                 the_renderables(),
                                                                 the_compositing_strategy());
        });
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::the_ipc_factory(
    std::shared_ptr<mf::Shell> const& shell,
    std::shared_ptr<mg::ViewableArea> const& display,
    std::shared_ptr<mc::GraphicBufferAllocator> const& allocator)
{
    return ipc_factory(
        [&]()
        {
            return std::make_shared<DefaultIpcFactory>(
                shell,
                the_session_mediator_report(),
                the_message_processor_report(),
                the_graphics_platform(),
                display, allocator);
        });
}

std::shared_ptr<mf::SessionMediatorReport>
mir::DefaultServerConfiguration::the_session_mediator_report()
{
    return session_mediator_report(
        [this]() -> std::shared_ptr<mf::SessionMediatorReport>
        {
            if (the_options()->get(log_app_mediator, false))
            {
                return std::make_shared<ml::SessionMediatorReport>(the_logger());
            }
            else
            {
                return std::make_shared<mf::NullSessionMediatorReport>();
            }
        });
}

std::shared_ptr<mf::MessageProcessorReport>
mir::DefaultServerConfiguration::the_message_processor_report()
{
    return message_processor_report(
        [this]() -> std::shared_ptr<mf::MessageProcessorReport>
        {
            if (the_options()->get(log_msg_processor, false))
            {
                return std::make_shared<ml::MessageProcessorReport>(the_logger(), the_time_source());
            }
            else
            {
                return std::make_shared<mf::NullMessageProcessorReport>();
            }
        });
}


std::shared_ptr<ml::Logger> mir::DefaultServerConfiguration::the_logger()
{
    return logger(
        [this]() -> std::shared_ptr<ml::Logger>
        {
            if (the_options()->is_set(glog))
            {
                return std::make_shared<ml::GlogLogger>(
                    "mir",
                    the_options()->get(glog_stderrthreshold, 2),
                    the_options()->get(glog_minloglevel, 0),
                    the_options()->get(glog_log_dir, ""));
            }
            else
            {
                return std::make_shared<ml::DumbConsoleLogger>();
            }
        });
}

std::shared_ptr<mi::InputChannelFactory> mir::DefaultServerConfiguration::the_input_channel_factory()
{
    return the_input_manager();
}

std::shared_ptr<msh::InputFocusSelector> mir::DefaultServerConfiguration::the_input_focus_selector()
{
    return the_input_manager();
}

std::shared_ptr<mir::time::TimeSource> mir::DefaultServerConfiguration::the_time_source()
{
    return time_source(
        []()
        {
            return std::make_shared<mir::time::HighResolutionClock>();
        });
}

std::shared_ptr<mir::MainLoop> mir::DefaultServerConfiguration::the_main_loop()
{
    return main_loop(
        []()
        {
            return std::make_shared<mir::AsioMainLoop>();
        });
}
