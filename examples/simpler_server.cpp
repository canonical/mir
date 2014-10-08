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

namespace mir
{
namespace options { class DefaultConfiguration; }

class SimplerServer
{
public:
    /// set a callback to introduce additional configuration options.
    /// this will be invoked by run() before server initialisation starts
    void set_add_configuration_options(
        std::function<void(options::DefaultConfiguration& config)> const& add_configuration_options);

    /// set a handler for any unrecognised command line options.
    /// The default action is to exit by throwing mir::AbnormalExit
    void set_command_line_hander(
        std::function<void(int argc, char const* const* argv)> const& command_line_hander);

    /// set the command line (this must remain valid while run() is called
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

private:
    std::function<void(options::DefaultConfiguration& config)> add_configuration_options{
        [](options::DefaultConfiguration&){}};
    std::function<void(int argc, char const* const* argv)> command_line_hander{};
    std::function<void()> init_callback{[](){}};
    int argc{0};
    char const** argv{nullptr};
    std::function<void()> exception_handler{};
};
}
// simple server header end

// simple server client start

#include "mir/options/default_configuration.h"

int main(int argc, char const* argv[])
{
    mir::SimplerServer simpler_server;

    simpler_server.set_add_configuration_options(
        [] (mir::options::DefaultConfiguration& config)
        {
            namespace po = boost::program_options;

            config.add_options()
                ("launch-client", po::value<std::string>(), "system() command to launch client");
        });

    simpler_server.set_command_line(argc, argv);
    simpler_server.run();
}
// simple server client end

// simple server implementation start

#include "mir/options/default_configuration.h"
#include "mir/default_server_configuration.h"
#include "mir/report_exception.h"
#include "mir/run_mir.h"

#include <iostream>

namespace mo = mir::options;

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

void mir::SimplerServer::set_add_configuration_options(
    std::function<void(mo::DefaultConfiguration& config)> const& add_configuration_options)
{
    this->add_configuration_options = add_configuration_options;
}


void mir::SimplerServer::set_command_line(int argc, char const* argv[])
{
	this->argc = argc;
	this->argv = argv;
}

void mir::SimplerServer::run()
try
{
    auto const options = configuration_options(argc, argv, command_line_hander);

    add_configuration_options(*options);

    DefaultServerConfiguration config{options};

    run_mir(config, [this](DisplayServer&) { init_callback(); });
}
catch (...)
{
    if (exception_handler)
        exception_handler();
    else
        mir::report_exception(std::cerr);
}

// simple server implementation end
