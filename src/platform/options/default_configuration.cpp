/*
 * Copyright © 2013-2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#include "mir/shared_library.h"
#include "mir/options/default_configuration.h"
#include "mir/graphics/platform.h"
#include "mir/default_configuration.h"
#include "mir/abnormal_exit.h"
#include "mir/shared_library_prober.h"
#include "mir/logging/null_shared_library_prober_report.h"

namespace mo = mir::options;

char const* const mo::server_socket_opt           = "file,f";
char const* const mo::prompt_socket_opt           = "prompt-file,p";
char const* const mo::no_server_socket_opt        = "no-file";
char const* const mo::arw_server_socket_opt       = "arw-file";
char const* const mo::enable_input_opt            = "enable-input,i";
char const* const mo::session_mediator_report_opt = "session-mediator-report";
char const* const mo::msg_processor_report_opt    = "msg-processor-report";
char const* const mo::compositor_report_opt       = "compositor-report";
char const* const mo::display_report_opt          = "display-report";
char const* const mo::legacy_input_report_opt     = "legacy-input-report";
char const* const mo::connector_report_opt        = "connector-report";
char const* const mo::scene_report_opt            = "scene-report";
char const* const mo::input_report_opt            = "input-report";
char const* const mo::seat_report_opt            = "seat-report";
char const* const mo::shared_library_prober_report_opt = "shared-library-prober-report";
char const* const mo::shell_report_opt            = "shell-report";
char const* const mo::name_opt                    = "name";
char const* const mo::offscreen_opt               = "offscreen";
char const* const mo::touchspots_opt              = "enable-touchspots";
char const* const mo::cursor_opt                  = "cursor";
char const* const mo::fatal_except_opt            = "on-fatal-error-except";
char const* const mo::debug_opt                   = "debug";
char const* const mo::composite_delay_opt         = "composite-delay";
char const* const mo::enable_key_repeat_opt       = "enable-key-repeat";
char const* const mo::x11_display_opt             = "enable-x11";
char const* const mo::wayland_extensions_opt      = "wayland-extensions";
char const* const mo::enable_mirclient_opt        = "enable-mirclient";

char const* const mo::off_opt_value = "off";
char const* const mo::log_opt_value = "log";
char const* const mo::lttng_opt_value = "lttng";

char const* const mo::platform_graphics_lib = "platform-graphics-lib";
char const* const mo::platform_input_lib = "platform-input-lib";
char const* const mo::platform_path = "platform-path";

char const* const mo::console_provider = "console-provider";
char const* const mo::logind_console = "logind";
char const* const mo::vt_console = "vt";
char const* const mo::null_console = "none";
char const* const mo::auto_console = "auto";

char const* const mo::vt_option_name = "vt";


namespace
{
bool const enable_input_default        = true;
}

mo::DefaultConfiguration::DefaultConfiguration(int argc, char const* argv[]) :
    DefaultConfiguration(argc, argv, std::string{})
{
}

mo::DefaultConfiguration::DefaultConfiguration(
    int argc,
    char const* argv[],
    std::string const& config_file) :
    DefaultConfiguration(argc, argv,
        [](int argc, char const* const* argv)
        {
            if (argc)
            {
                std::ostringstream help_text;
                help_text << "Unknown command line options:";
                for (auto opt = argv; opt != argv+argc ; ++opt)
                    help_text << ' ' << *opt;
                BOOST_THROW_EXCEPTION(mir::AbnormalExit(help_text.str()));
            }
        },
        config_file)
{
}

mo::DefaultConfiguration::DefaultConfiguration(
    int argc,
    char const* argv[],
    std::function<void(int argc, char const* const* argv)> const& handler) :
    DefaultConfiguration(argc, argv, handler, std::string{})
{
}

namespace
{
std::string description_text(char const* program, std::string const& config_file)
{
    std::string result{
        "Command-line options (e.g. \"--host-socket=/tmp/mir_socket\").\n\n"
        "Environment variables capitalise long form with prefix \"MIR_SERVER_\" and \"_\" in place of \"-\".\n"
        "(E.g. \"MIR_SERVER_HOST_SOCKET=/tmp/mir_socket\")\n\n"};

    if (program)
        result = std::string{"usage: "} + program + " [options]\n\n" + result;

    if (!config_file.empty())
        result +=
        "Config file entries are long form (e.g. \"host-socket=/tmp/mir_socket\").\n"
        "The config file (" + config_file + ") is located via the XDG Base Directory Specification.\n"
        "($XDG_CONFIG_HOME or $HOME/.config followed by $XDG_CONFIG_DIRS)\n\n";

    return result + "user options";
}
}

mo::DefaultConfiguration::DefaultConfiguration(
    int argc,
    char const* argv[],
    std::function<void(int argc, char const* const* argv)> const& handler,
    std::string const& config_file) :
    config_file{config_file},
    argc(argc),
    argv(argv),
    unparsed_arguments_handler{handler},
    program_options(std::make_shared<boost::program_options::options_description>(
        description_text(argv[0], config_file)))
{
    using namespace options;
    namespace po = boost::program_options;

    add_options()
        (server_socket_opt, po::value<std::string>()->default_value(::mir::default_server_socket),
            "Socket filename [string:default=$XDG_RUNTIME_DIR/mir_socket or /tmp/mir_socket]")
        (no_server_socket_opt, "Do not provide a socket filename for client connections")
        (arw_server_socket_opt, "Make socket filename globally rw (equivalent to chmod a=rw)")
        (prompt_socket_opt, "Provide a \"..._trusted\" filename for prompt helper connections")
        (platform_graphics_lib, po::value<std::string>(),
            "Library to use for platform graphics support (default: autodetect)")
        (platform_input_lib, po::value<std::string>(),
            "Library to use for platform input support (default: input-stub.so)")
        (platform_path, po::value<std::string>()->default_value(MIR_SERVER_PLATFORM_PATH),
            "Directory to look for platform libraries (default: " MIR_SERVER_PLATFORM_PATH ")")
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
        (seat_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle to Seat report. [{log,off}]")
        (session_mediator_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the SessionMediator report. [{log,lttng,off}]")
        (msg_processor_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the MessageProcessor report. [{log,lttng,off}]")
        (scene_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "How to handle the scene report. [{log,lttng,off}]")
        (shared_library_prober_report_opt, po::value<std::string>()->default_value(log_opt_value),
            "How to handle the SharedLibraryProber report. [{log,lttng,off}]")
        (shell_report_opt, po::value<std::string>()->default_value(off_opt_value),
         "How to handle the Shell report. [{log,off}]")
        (composite_delay_opt, po::value<int>()->default_value(0),
            "Compositor frame delay in milliseconds (how long to wait for new "
            "frames from clients before compositing). Higher values result in "
            "lower latency but risk causing frame skipping. "
            "Default: A negative value means decide automatically.")
        (name_opt, po::value<std::string>(),
            "When nested, the name Mir uses when registering with the host.")
        (offscreen_opt,
            "Render to offscreen buffers instead of the real outputs.")
        (touchspots_opt,
            "Display visualization of touchspots (e.g. for screencasting).")
        (cursor_opt,
            po::value<std::string>()->default_value("auto"),
            "Cursor (mouse pointer) to use [{auto,null,software}]")
        (enable_key_repeat_opt, po::value<bool>()->default_value(true),
             "Enable server generated key repeat")
        (fatal_except_opt, "On \"fatal error\" conditions [e.g. drivers behaving "
            "in unexpected ways] throw an exception (instead of a core dump)")
        (debug_opt, "Enable extra development debugging. "
            "This is only interesting for people doing Mir server or client development.")
        (enable_mirclient_opt, "Enable deprecated mirclient socket (for running old clients)")
        (console_provider,
            po::value<std::string>()->default_value("auto"),
            "Console device handling\n"
            "How Mir handles console-related tasks (device handling, VT switching, etc)\n"
            "Options:\n"
            "logind: use logind\n"
            "vt: use the Linux VT subsystem. Requires root.\n"
            "none: support no console-related tasks. Useful for nested platforms which do not "
                "need raw device access and which don't have a VT concept\n"
            "auto: detect the appropriate provider.")
        (vt_option_name,
            boost::program_options::value<int>()->default_value(0),
            "[requires --console-provider=vt] VT to run on or 0 to use current.");

        add_platform_options();
}

void mo::DefaultConfiguration::add_platform_options()
{
    namespace po = boost::program_options;
    po::options_description program_options;
    program_options.add_options()
        (platform_graphics_lib,
         po::value<std::string>(), "");
    program_options.add_options()
        (platform_path,
         po::value<std::string>()->default_value(MIR_SERVER_PLATFORM_PATH),
        "");
    mo::ProgramOption options;
    options.parse_arguments(program_options, argc, argv);

    try
    {
        auto env_libname = ::getenv("MIR_SERVER_PLATFORM_GRAPHICS_LIB");
        auto env_libpath = ::getenv("MIR_SERVER_PLATFORM_PATH");

        platform_libraries =
            [env_libname, env_libpath, &options]()
            {
                if (options.is_set(platform_graphics_lib))
                {
                    return std::vector<std::shared_ptr<mir::SharedLibrary>>{
                        std::make_shared<mir::SharedLibrary>(
                            options.get<std::string>(platform_graphics_lib))};
                }
                else if (env_libname)
                {
                    return std::vector<std::shared_ptr<mir::SharedLibrary>>{
                        std::make_shared<mir::SharedLibrary>(std::string{env_libname})
                    };
                }
                else
                {
                    mir::logging::NullSharedLibraryProberReport null_report;
                    auto const plugin_path = env_libpath ? env_libpath : options.get<std::string>(platform_path);
                    return mir::libraries_for_path(plugin_path, null_report);
                }
            }();

        for (auto& platform : platform_libraries)
        {
            /* Ideally we'd namespace these options with the platform,
             * and display them in a group as $FOO-platform-specific.
             */
            try
            {
                auto add_platform_options = platform->load_function<mir::graphics::AddPlatformOptions>("add_graphics_platform_options", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                add_platform_options(*this->program_options);
            }
            catch (std::runtime_error&)
            {
                /* We've failed to add the options - probably because it's not a graphics platform,
                 * or because it's got the wrong version - unload it; it's unnecessary.
                 */
                platform.reset();
            }
        }

        // Remove the shared_ptrs to the libraries we've unloaded from the vector.
        platform_libraries.erase(
            std::remove(
                platform_libraries.begin(),
                platform_libraries.end(),
                std::shared_ptr<mir::SharedLibrary>{}),
            platform_libraries.end());
    }
    catch(...)
    {
        // We don't actually care at this point if this failed.
        // Maybe we've been pointed at the wrong place. Maybe this platform doesn't actually
        // *have* platform-specific options.
        // Regardless, if we need a platform and can't find one then we'll bail later
        // in startup with a useful error.
    }
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

        auto const unparsed_arguments = options.unparsed_command_line();
        std::vector<char const*> tokens;
        for (auto const& token : unparsed_arguments)
            tokens.push_back(token.c_str());
        if (!tokens.empty()) unparsed_arguments_handler(tokens.size(), tokens.data());

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
    options.parse_environment(desc, "MIR_SERVER_");
}

void mo::DefaultConfiguration::parse_config_file(
    boost::program_options::options_description& desc,
    mo::ProgramOption& options) const
{
    if (!config_file.empty())
        options.parse_file(desc, config_file);
}
