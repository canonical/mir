/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/options/default_configuration.h"
#include "mir/default_configuration.h"
#include "mir/abnormal_exit.h"

namespace mo = mir::options;

char const* const mo::server_socket_opt           = "file,f";
char const* const mo::no_server_socket_opt        = "no-file";
char const* const mo::enable_input_opt            = "enable-input,i";
char const* const mo::session_mediator_report_opt = "session-mediator-report";
char const* const mo::msg_processor_report_opt    = "msg-processor-report";
char const* const mo::compositor_report_opt       = "compositor-report";
char const* const mo::display_report_opt          = "display-report";
char const* const mo::legacy_input_report_opt     = "legacy-input-report";
char const* const mo::connector_report_opt        = "connector-report";
char const* const mo::scene_report_opt            = "scene-report";
char const* const mo::input_report_opt            = "input-report";
char const* const mo::host_socket_opt             = "host-socket";
char const* const mo::frontend_threads_opt        = "ipc-thread-pool";
char const* const mo::name_opt                    = "name";
char const* const mo::offscreen_opt               = "offscreen";

char const* const mo::glog                 = "glog";
char const* const mo::glog_stderrthreshold = "glog-stderrthreshold";
char const* const mo::glog_minloglevel     = "glog-minloglevel";
char const* const mo::glog_log_dir         = "glog-log-dir";
char const* const mo::off_opt_value = "off";
char const* const mo::log_opt_value = "log";
char const* const mo::lttng_opt_value = "lttng";

char const* const mo::platform_graphics_lib = "platform-graphics-lib";

namespace
{
int const default_ipc_threads          = 1;
int const glog_stderrthreshold_default = 2;
int const glog_minloglevel_default     = 0;
char const* const glog_log_dir_default = "";
bool const enable_input_default        = true;
char const* const default_platform_graphics_lib = "libmirplatformgraphics.so";
}

mo::DefaultConfiguration::DefaultConfiguration(int argc, char const* argv[]) :
    argc(argc),
    argv(argv),
    program_options(std::make_shared<boost::program_options::options_description>(
    "Command-line options.\n"
    "Environment variables capitalise long form with prefix \"MIR_SERVER_\" and \"_\" in place of \"-\""))
{
    using namespace options;
    namespace po = boost::program_options;

    add_options()
        (host_socket_opt, po::value<std::string>(),
            "Host socket filename. [string:default={$MIR_SOCKET,$XDG_RUNTIME_DIR/mir_socket}]")
        (server_socket_opt, po::value<std::string>()->default_value(::mir::default_server_socket),
            "Socket filename.")
        (no_server_socket_opt, "Do not provide a socket filename for client connections")
        (platform_graphics_lib, po::value<std::string>()->default_value(default_platform_graphics_lib),
            "Library to use for platform graphics support")
        (enable_input_opt, po::value<bool>()->default_value(enable_input_default),
            "Enable input.")
        (compositor_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "Compositor reporting [{log,lttng,off}]")
        (connector_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the Connector report. [{log,lttng,off}]")
        (display_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the Display report. [{log,lttng,off}]")
        (input_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle to Input report. [{log,lttng,off}]")
        (legacy_input_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the Legacy Input report. [{log,off}]")
        (session_mediator_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the SessionMediator report. [{log,lttng,off}]")
        (msg_processor_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the MessageProcessor report. [{log,lttng,off}]")
        (scene_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the scene report. [{log,lttng,off}]")
        (glog,
            "Use google::GLog for logging")
        (glog_stderrthreshold, po::value<int>()->default_value(glog_stderrthreshold_default),
            "Copy log messages at or above this level "
            "to stderr in addition to logfiles. The numbers "
            "of severity levels INFO, WARNING, ERROR, and "
            "FATAL are 0, 1, 2, and 3, respectively.")
        (glog_minloglevel, po::value<int>()->default_value(glog_minloglevel_default),
            "Log messages at or above this level. The numbers "
            "of severity levels INFO, WARNING, ERROR, and "
            "FATAL are 0, 1, 2, and 3, respectively."
            " [int:default=0]")
        (glog_log_dir, po::value<std::string>()->default_value(glog_log_dir_default),
            "If specified, logfiles are written into this "
            "directory instead of the default logging directory.")
        (frontend_threads_opt, po::value<int>()->default_value(default_ipc_threads),
            "threads in frontend thread pool.")
        (name_opt, po::value<std::string>(),
            "When nested, the name Mir uses when registering with the host.")
        (offscreen_opt,
            "Render to offscreen buffers instead of the real outputs.")
        ("vt", po::value<int>()->default_value(0), // TODO this not applicable on all graphics platforms
            "VT to run on or 0 to use current.");
}

boost::program_options::options_description_easy_init mo::DefaultConfiguration::add_options()
{
    if (options)
        BOOST_THROW_EXCEPTION(std::logic_error("add_options() must be called before the_options()"));

    return program_options->add_options();
}

std::shared_ptr<mo::Option> mo::DefaultConfiguration::the_options() const
{
    if (!options)
    {
        auto options = std::make_shared<ProgramOption>();
        parse_arguments(*program_options, *options, argc, argv);
        parse_environment(*program_options, *options);
        parse_config_file(*program_options, *options);
        this->options = options;
    }
    return options;
}


void mo::DefaultConfiguration::parse_arguments(
    boost::program_options::options_description desc,
    mo::ProgramOption& options,
    int argc,
    char const* argv[]) const
{
    namespace po = boost::program_options;

    try
    {
        desc.add_options()
            ("help,h", "this help text");

        options.parse_arguments(desc, argc, argv);

        if (options.is_set("help"))
        {
            std::ostringstream help_text;
            help_text << desc;
            BOOST_THROW_EXCEPTION(mir::AbnormalExit(help_text.str()));
        }
    }
    catch (po::error const& error)
    {
        std::ostringstream help_text;
        help_text << "Failed to parse command line options: " << error.what() << "." << std::endl << desc;
        BOOST_THROW_EXCEPTION(mir::AbnormalExit(help_text.str()));
    }
}

void mo::DefaultConfiguration::parse_environment(
    boost::program_options::options_description& desc,
    mo::ProgramOption& options) const
{
    // If MIR_SERVER_HOST_SOCKET is unset, we want to substitute the value of
    // MIR_SOCKET.  Do this now, because MIR_SOCKET will get overwritten later.
    auto host_socket = getenv("MIR_SERVER_HOST_SOCKET");
    auto mir_socket = getenv("MIR_SOCKET");
    if (!host_socket && mir_socket)
        setenv("MIR_SERVER_HOST_SOCKET", mir_socket, 1);

    options.parse_environment(desc, "MIR_SERVER_");
}

void mo::DefaultConfiguration::parse_config_file(
    boost::program_options::options_description& /*desc*/,
    mo::ProgramOption& /*options*/) const
{
}
