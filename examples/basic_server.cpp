/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "example_input_event_filter.h"
#include "example_display_configuration_policy.h"
#include "glog_logger.h"

#include "mir/server.h"
#include "mir/report_exception.h"
#include "mir/options/option.h"

#include <cstdlib>

namespace me = mir::examples;
namespace ml = mir::logging;
namespace mg = mir::graphics;

namespace
{
void add_glog_options_to(mir::Server& server)
{
    static char const* const glog                 = "glog";
    static char const* const glog_stderrthreshold = "glog-stderrthreshold";
    static char const* const glog_minloglevel     = "glog-minloglevel";
    static char const* const glog_log_dir         = "glog-log-dir";

    static int const glog_stderrthreshold_default = 2;
    static int const glog_minloglevel_default     = 0;
    static char const* const glog_log_dir_default = "";

    server.add_configuration_option(glog, "Use google::GLog for logging", mir::OptionType::null);
    server.add_configuration_option(glog_stderrthreshold,
        "Copy log messages at or above this level "
        "to stderr in addition to logfiles. The numbers "
        "of severity levels INFO, WARNING, ERROR, and "
        "FATAL are 0, 1, 2, and 3, respectively.",
        glog_stderrthreshold_default);
    server.add_configuration_option(glog_minloglevel,
        "Log messages at or above this level. The numbers "
        "of severity levels INFO, WARNING, ERROR, and "
        "FATAL are 0, 1, 2, and 3, respectively.",
        glog_minloglevel_default);
    server.add_configuration_option(glog_log_dir, "logfiles are written into this directory.", glog_log_dir_default);

    server.override_the_logger(
        [&]() -> std::shared_ptr<ml::Logger>
        {
            if (server.get_options()->is_set(glog))
            {
                return std::make_shared<me::GlogLogger>(
                    "mir",
                    server.get_options()->get<int>(glog_stderrthreshold),
                    server.get_options()->get<int>(glog_minloglevel),
                    server.get_options()->get<std::string>(glog_log_dir));
            }
            else
            {
                return std::shared_ptr<ml::Logger>{};
            }
        });
}

void add_launcher_option_to(mir::Server& server)
{
    static const char* const launch_child_opt = "launch-client";
    static const char* const launch_client_descr = "system() command to launch client";

    server.add_configuration_option(launch_child_opt, launch_client_descr, mir::OptionType::string);
    server.add_init_callback([&]
    {
        const auto options = server.get_options();
        if (options->is_set(launch_child_opt))
        {
            auto ignore = std::system((options->get<std::string>(launch_child_opt) + "&").c_str());
            (void)(ignore);
        }
    });
}
}

int main(int argc, char const* argv[])
try
{
    mir::Server server;

    auto const quit_filter = me::make_quit_filter_for(server);

    me::add_display_configuration_options_to(server);
    add_glog_options_to(server);
    add_launcher_option_to(server);

    // Provide the command line and run the server
    server.set_command_line(argc, argv);
    server.apply_settings();
    server.run();
    return server.exited_normally() ? EXIT_SUCCESS : EXIT_FAILURE;
}
catch (...)
{
    mir::report_exception();
    return EXIT_FAILURE;
}
