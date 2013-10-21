/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_options.h"
#include "mir/abnormal_exit.h"

namespace
{
void parse_arguments(
    boost::program_options::options_description desc,
    mir::options::ProgramOption& options,
    int argc,
    char const* argv[])
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

void parse_environment(
    boost::program_options::options_description& desc,
    mir::options::ProgramOption& options)
{
    // If MIR_SERVER_HOST_SOCKET is unset, we want to substitute the value of
    // MIR_SOCKET.  Do this now, because MIR_SOCKET will get overwritten later.
    auto host_socket = getenv("MIR_SERVER_HOST_SOCKET");
    auto mir_socket = getenv("MIR_SOCKET");
    if (!host_socket && mir_socket)
        setenv("MIR_SERVER_HOST_SOCKET", mir_socket, 1);

    options.parse_environment(desc, "MIR_SERVER_");
}
}

char const* const mir::DefaultOptions::server_socket_opt           = "file";
char const* const mir::DefaultOptions::no_server_socket_opt        = "no-file";
char const* const mir::DefaultOptions::session_mediator_report_opt = "session-mediator-report";
char const* const mir::DefaultOptions::msg_processor_report_opt    = "msg-processor-report";
char const* const mir::DefaultOptions::display_report_opt          = "display-report";
char const* const mir::DefaultOptions::legacy_input_report_opt     = "legacy-input-report";
char const* const mir::DefaultOptions::connector_report_opt        = "connector-report";
char const* const mir::DefaultOptions::input_report_opt            = "input-report";
char const* const mir::DefaultOptions::host_socket_opt             = "host-socket";
char const* const mir::DefaultOptions::standalone_opt              = "standalone";

char const* const mir::DefaultOptions::glog                 = "glog";
char const* const mir::DefaultOptions::glog_stderrthreshold = "glog-stderrthreshold";
char const* const mir::DefaultOptions::glog_minloglevel     = "glog-minloglevel";
char const* const mir::DefaultOptions::glog_log_dir         = "glog-log-dir";

bool const mir::DefaultOptions::enable_input_default = true;

char const* const mir::DefaultOptions::off_opt_value = "off";
char const* const mir::DefaultOptions::log_opt_value = "log";
char const* const mir::DefaultOptions::lttng_opt_value = "lttng";

char const* const mir::DefaultOptions::platform_graphics_lib = "platform-graphics-lib";
char const* const mir::DefaultOptions::default_platform_graphics_lib = "libmirplatformgraphics.so";


mir::DefaultOptions::DefaultOptions(int argc, char const* argv[]) :
    argc(argc),
    argv(argv),
    program_options(std::make_shared<boost::program_options::options_description>(
    "Command-line options.\n"
    "Environment variables capitalise long form with prefix \"MIR_SERVER_\" and \"_\" in place of \"-\""))
{
    namespace po = boost::program_options;

    add_options()
        (standalone_opt, po::value<bool>(),
            "Run mir in standalone mode. [bool:default=false]")
        (host_socket_opt, po::value<std::string>(),
            "Host socket filename. [string:default={$MIR_SOCKET,$XDG_RUNTIME_DIR/mir_socket}]")
        ("file,f", po::value<std::string>(),
            "Socket filename. [string:default=$XDG_RUNTIME_DIR/mir_socket]")
        (no_server_socket_opt, "Do not provide a socket filename for client connections")
        (platform_graphics_lib, po::value<std::string>(),
            "Library to use for platform graphics support [default=libmirplatformgraphics.so]")
        ("enable-input,i", po::value<bool>(),
            "Enable input. [bool:default=true]")
        (connector_report_opt, po::value<std::string>(),
            "How to handle the Connector report. [{log,off}:default=off]")
        (display_report_opt, po::value<std::string>(),
            "How to handle the Display report. [{log,off}:default=off]")
        (input_report_opt, po::value<std::string>(),
            "How to handle to Input report. [{log,lttng,off}:default=off]")
        (legacy_input_report_opt, po::value<std::string>(),
            "How to handle the Legacy Input report. [{log,off}:default=off]")
        (session_mediator_report_opt, po::value<std::string>(),
            "How to handle the SessionMediator report. [{log,off}:default=off]")
        (msg_processor_report_opt, po::value<std::string>(),
            "How to handle the MessageProcessor report. [{log,lttng,off}:default=off]")
        (glog,
            "Use google::GLog for logging")
        (glog_stderrthreshold, po::value<int>(),
            "Copy log messages at or above this level "
            "to stderr in addition to logfiles. The numbers "
            "of severity levels INFO, WARNING, ERROR, and "
            "FATAL are 0, 1, 2, and 3, respectively."
            " [int:default=2]")
        (glog_minloglevel, po::value<int>(),
            "Log messages at or above this level. The numbers "
            "of severity levels INFO, WARNING, ERROR, and "
            "FATAL are 0, 1, 2, and 3, respectively."
            " [int:default=0]")
        (glog_log_dir, po::value<std::string>(),
            "If specified, logfiles are written into this "
            "directory instead of the default logging directory."
            " [string:default=\"\"]")
        ("ipc-thread-pool", po::value<int>(),
            "threads in frontend thread pool.")
        ("vt", po::value<int>(),
            "VT to run on or 0 to use current. [int:default=0]");
}

boost::program_options::options_description_easy_init mir::DefaultOptions::add_options()
{
    if (options)
        BOOST_THROW_EXCEPTION(std::logic_error("add_options() must be called before the_options()"));

    return program_options->add_options();
}

void mir::DefaultOptions::parse_options(boost::program_options::options_description& options_description, mir::options::ProgramOption& options) const
{
    parse_arguments(options_description, options, argc, argv);
    parse_environment(options_description, options);
}

std::shared_ptr<mir::options::Option> mir::DefaultOptions::the_options() const
{
    if (!options)
    {
        auto options = std::make_shared<mir::options::ProgramOption>();
        parse_options(*program_options, *options);
        this->options = options;
    }
    return options;
}
