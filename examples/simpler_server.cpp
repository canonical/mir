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
class SimplerServer
{
public:
    void set_command_line_hander(
        std::function<void(int argc, char const* const* argv)> const& command_line_hander);

    void set_command_line(int argc, char const* const argv[]);

    void set_init_callback(std::function<void()> const& init_callback);

    void run();

    void stop();

private:
    std::function<void(int argc, char const* const* argv)> command_line_hander{};
    std::function<void()> init_callback{};
    int argc{0};
    char const** argv{nullptr};
};
}
// simple server header end

// simple server client start
int main(int argc, char const* argv[])
{
    (void)argc;
    (void)argv;

    mir::SimplerServer simpler_server;

    simpler_server.run();
}
// simple server client end

// simple server implementation start

#include "mir/options/default_configuration.h"
#include "mir/default_server_configuration.h"
#include "mir/run_mir.h"

namespace mo = mir::options;

namespace
{
std::shared_ptr<mo::Configuration> configuration_options(
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

void mir::SimplerServer::run()
{
    auto const options = configuration_options(argc, argv, command_line_hander);

    DefaultServerConfiguration config{options};

    run_mir(config, [&](DisplayServer&) { if (init_callback) init_callback(); });
}

// simple server implementation end
