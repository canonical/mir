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
#include "mir/input/composite_event_filter.h"

#include <cstdlib>

namespace me = mir::examples;
namespace ml = mir::logging;
namespace mg = mir::graphics;

namespace
{
char const* const glog                 = "glog";
char const* const glog_stderrthreshold = "glog-stderrthreshold";
char const* const glog_minloglevel     = "glog-minloglevel";
char const* const glog_log_dir         = "glog-log-dir";

int const glog_stderrthreshold_default = 2;
int const glog_minloglevel_default     = 0;
char const* const glog_log_dir_default = "";
}

int main(int argc, char const* argv[])
try
{
    static char const* const launch_child_opt = "launch-client";
    static char const* const launch_client_descr = "system() command to launch client";

    mir::Server server;

    // Set up a Ctrl+Alt+BkSp => quit
    auto const quit_filter = std::make_shared<me::QuitFilter>([&]{ server.stop(); });

    server.add_init_callback([&] { server.the_composite_event_filter()->append(quit_filter); });

    // Add choice of monitor configuration
    server.add_configuration_option(
        me::display_config_opt, me::display_config_descr,   me::clone_opt_val);
    server.add_configuration_option(
        me::display_alpha_opt,  me::display_alpha_descr,    me::display_alpha_off);

    server.add_configuration_option(glog, "Use google::GLog for logging", mir::OptionType::null);

    server.add_configuration_option(
        glog_stderrthreshold,
        "Copy log messages at or above this level "
        "to stderr in addition to logfiles. The numbers "
        "of severity levels INFO, WARNING, ERROR, and "
        "FATAL are 0, 1, 2, and 3, respectively.",
        glog_stderrthreshold_default);

    server.add_configuration_option(
        glog_minloglevel,
        "Log messages at or above this level. The numbers "
        "of severity levels INFO, WARNING, ERROR, and "
        "FATAL are 0, 1, 2, and 3, respectively.",
        glog_minloglevel_default);

    server.add_configuration_option(
        glog_log_dir,
        "logfiles are written into this directory.",
        glog_log_dir_default);

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

    server.wrap_display_configuration_policy(
        [&](std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto const options = server.get_options();
            auto display_layout = options->get<std::string>(me::display_config_opt);
            auto with_alpha = options->get<std::string>(me::display_alpha_opt) == me::display_alpha_on;

            auto layout_selector = wrapped;

            if (display_layout == me::sidebyside_opt_val)
                layout_selector = std::make_shared<me::SideBySideDisplayConfigurationPolicy>();
            else if (display_layout == me::single_opt_val)
                layout_selector = std::make_shared<me::SingleDisplayConfigurationPolicy>();

            // Whatever the layout select a pixel format with requested alpha
            return std::make_shared<me::PixelFormatSelector>(layout_selector, with_alpha);
        });

    // Add a launcher option
    server.add_configuration_option(
        launch_child_opt,       launch_client_descr,        mir::OptionType::string);

    server.add_init_callback([&]
        {
            auto const options = server.get_options();

            if (options->is_set(launch_child_opt))
            {
                auto ignore = std::system((options->get<std::string>(launch_child_opt) + "&").c_str());
                (void)ignore;
            }
        });

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
