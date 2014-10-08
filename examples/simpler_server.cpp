/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

// simple server header start
#include <functional>
#include <memory>

namespace mir
{
namespace options { class DefaultConfiguration; class Option; }
namespace graphics { class Platform; class Display; }
namespace input { class CompositeEventFilter; }

class MainLoop;

class Server
{
public:
    /// set a callback to introduce additional configuration options.
    /// this will be invoked by run() before server initialisation starts
    void set_add_configuration_options(
        std::function<void(options::DefaultConfiguration& config)> const& add_configuration_options);

    /// set a handler for any command line options Mir does not recognise.
    /// This will be invoked if any unrecognised options are found during initialisation.
    /// Any unrecognised arguments are passed to this function. The pointers remain valid
    /// for the duration of the call only.
    /// If set_command_line_hander is not called the default action is to exit by
    /// throwing mir::AbnormalExit (which will be handled by the exception handler prior to
    /// exiting run().
    void set_command_line_hander(
        std::function<void(int argc, char const* const* argv)> const& command_line_hander);

    /// set the command line (this must remain valid while run() is called)
    void set_command_line(int argc, char const* argv[]);

    /// set a callback to be invoked when the server has been initialized,
    /// but before it starts. This allows client code to get access Mir objects
    void set_init_callback(std::function<void()> const& init_callback);

    /// Set a handler for exceptions. This is invoked in a catch (...) block and
    /// the exception can be re-thrown to retrieve type information.
    /// The default action is to call mir::report_exception(std::cerr)
    void set_exception_handler(std::function<void()> const& exception_handler);

    /// Run the Mir server until it exits
    void run();

    /// Tell the Mir server to exit
    void stop();

    /// returns true if and only if server exited normally. Otherwise false.
    bool exited_normally();

    /// Returns the configuration options.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto get_options() const -> std::shared_ptr<options::Option>;

    /// Returns the graphics platform options.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_graphics_platform() const -> std::shared_ptr<graphics::Platform>;

    /// Returns the graphics display options.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_display() const -> std::shared_ptr<graphics::Display>;

    /// Returns the main loop.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_main_loop() const -> std::shared_ptr<MainLoop>;

    /// Returns the composite event filter.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_composite_event_filter() const -> std::shared_ptr<input::CompositeEventFilter>;

private:
    std::function<void(options::DefaultConfiguration& config)> add_configuration_options{
        [](options::DefaultConfiguration&){}};
    std::function<void(int argc, char const* const* argv)> command_line_hander{};
    std::function<void()> init_callback{[]{}};
    int argc{0};
    char const** argv{nullptr};
    std::function<void()> exception_handler{};
    bool exit_status{false};
    std::weak_ptr<options::Option> options;
    struct DefaultServerConfiguration;
    DefaultServerConfiguration* server_config{nullptr};
};
}
// simple server header end

// simple server client start

#include "mir/options/default_configuration.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/input/composite_event_filter.h"
#include "graphics.h"

#include <linux/input.h>

#include <cstdlib>
#include <csignal>

namespace
{
class ExampleExit {};

volatile std::sig_atomic_t running = true;

void signal_handler(int /*signum*/)
{
    running = false;
}

void render_to_fb(std::shared_ptr<mir::graphics::Display> const& display)
{
    /* Set up graceful exit on SIGINT and SIGTERM */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    mir::draw::glAnimationBasic gl_animation;

    display->for_each_display_buffer([&](mir::graphics::DisplayBuffer& buffer)
    {
        buffer.make_current();
        gl_animation.init_gl();
    });

    while (running)
    {
        display->for_each_display_buffer([&](mir::graphics::DisplayBuffer& buffer)
        {
            buffer.make_current();

            gl_animation.render_gl();

            buffer.post_update();
        });

        gl_animation.step();
    }
}

class QuitFilter : public mir::input::EventFilter
{
public:
    QuitFilter(mir::Server& server)
        : server{server}
    {
    }

    bool handle(MirEvent const& event) override
    {
        if (event.type == mir_event_type_key &&
            event.key.action == mir_key_action_down &&
            (event.key.modifiers & mir_key_modifier_alt) &&
            (event.key.modifiers & mir_key_modifier_ctrl) &&
            event.key.scan_code == KEY_BACKSPACE)
        {
            server.stop();
            return true;
        }

        return false;
    }

private:
    mir::Server& server;
};
}

int main(int argc, char const* argv[])
{
    static char const* const launch_child_opt = "launch-client";
    static char const* const render_to_fb_opt = "render-to-fb";
    mir::Server server;

    auto const quit_filter = std::make_shared<QuitFilter>(server);

    server.set_add_configuration_options(
        [] (mir::options::DefaultConfiguration& config)
        {
            namespace po = boost::program_options;

            config.add_options()
                (render_to_fb_opt, "emulate render_to_fb")
                (launch_child_opt, po::value<std::string>(), "system() command to launch client");
        });

    server.set_command_line(argc, argv);
    server.set_init_callback([&]
        {
            server.the_composite_event_filter()->append(quit_filter);

            auto const options = server.get_options();

            if (options->is_set(render_to_fb_opt))
            {
                render_to_fb(server.the_display());

                server.set_exception_handler([]{});
                throw ExampleExit{};
            }
            else if (options->is_set(launch_child_opt))
            {
                auto ignore = std::system((options->get<std::string>(launch_child_opt) + "&").c_str());
                (void)ignore;
            }
        });

    server.run();

    return server.exited_normally() ? EXIT_SUCCESS : EXIT_FAILURE;
}
// simple server client end

// simple server implementation start

#include "mir/options/default_configuration.h"
#include "mir/default_server_configuration.h"
#include "mir/main_loop.h"
#include "mir/report_exception.h"
#include "mir/run_mir.h"

#include <iostream>

namespace mo = mir::options;

struct mir::Server::DefaultServerConfiguration : mir::DefaultServerConfiguration
{
    using mir::DefaultServerConfiguration::DefaultServerConfiguration;
    using mir::DefaultServerConfiguration::the_options;
};

namespace
{
std::shared_ptr<mo::DefaultConfiguration> configuration_options(
    int argc,
    char const** argv,
    std::function<void(int argc, char const* const* argv)> const& command_line_hander)
{
    if (command_line_hander)
        return std::make_shared<mo::DefaultConfiguration>(argc, argv, command_line_hander);
    else
        return std::make_shared<mo::DefaultConfiguration>(argc, argv);

}
}

void mir::Server::set_add_configuration_options(
    std::function<void(mo::DefaultConfiguration& config)> const& add_configuration_options)
{
    this->add_configuration_options = add_configuration_options;
}


void mir::Server::set_command_line(int argc, char const* argv[])
{
	this->argc = argc;
	this->argv = argv;
}

void mir::Server::set_init_callback(std::function<void()> const& init_callback)
{
    this->init_callback = init_callback;
}

auto mir::Server::get_options() const -> std::shared_ptr<options::Option>
{
    return options.lock();
}

void mir::Server::set_exception_handler(std::function<void()> const& exception_handler)
{
    this->exception_handler = exception_handler;
}

void mir::Server::run()
try
{
    auto const options = configuration_options(argc, argv, command_line_hander);

    add_configuration_options(*options);

    DefaultServerConfiguration config{options};

    server_config = &config;

    run_mir(config, [&](DisplayServer&)
        {
            this->options = config.the_options();
            init_callback();
        });

    exit_status = true;
    server_config = nullptr;
}
catch (...)
{
    server_config = nullptr;

    if (exception_handler)
        exception_handler();
    else
        mir::report_exception(std::cerr);
}

void mir::Server::stop()
{
    std::cerr << "DEBUG: " << __PRETTY_FUNCTION__ << std::endl;
    if (auto const main_loop = the_main_loop())
        main_loop->stop();
}

bool mir::Server::exited_normally()
{
    return exit_status;
}

auto mir::Server::the_graphics_platform() const -> std::shared_ptr<graphics::Platform>
{
    if (server_config) return server_config->the_graphics_platform();

    return {};
}

auto mir::Server::the_display() const -> std::shared_ptr<graphics::Display>
{
    if (server_config) return server_config->the_display();

    return {};
}

auto mir::Server::the_main_loop() const -> std::shared_ptr<MainLoop>
{
    if (server_config) return server_config->the_main_loop();

    return {};
}

auto mir::Server::the_composite_event_filter() const
-> std::shared_ptr<input::CompositeEventFilter>
{
    if (server_config) return server_config->the_composite_event_filter();

    return {};
}

// simple server implementation end
