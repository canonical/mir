/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/options/program_option.h"
#include "mir/shared_library.h"
#include "mir/options/default_configuration.h"
#include "mir/graphics/platform.h"
#include "mir/abnormal_exit.h"
#include "mir/shared_library_prober.h"
#include "mir/logging/null_shared_library_prober_report.h"

#include <format>
#include <iostream>

namespace mo = mir::options;

char const* const mo::arw_server_socket_opt       = "arw-file";
char const* const mo::enable_input_opt            = "enable-input,i";
char const* const mo::compositor_report_opt       = "compositor-report";
char const* const mo::display_report_opt          = "display-report";
char const* const mo::scene_report_opt            = "scene-report";
char const* const mo::input_report_opt            = "input-report";
char const* const mo::seat_report_opt            = "seat-report";
char const* const mo::shared_library_prober_report_opt = "shared-library-prober-report";
char const* const mo::shell_report_opt            = "shell-report";
char const* const mo::touchspots_opt              = "enable-touchspots";
char const* const mo::cursor_opt                  = "cursor";
char const* const mo::fatal_except_opt            = "on-fatal-error-except";
char const* const mo::debug_opt                   = "debug";
char const* const mo::composite_delay_opt         = "composite-delay";
char const* const mo::enable_key_repeat_opt       = "enable-key-repeat";
char const* const mo::x11_display_opt             = "enable-x11";
char const* const mo::x11_scale_opt               = "x11-scale";
char const* const mo::wayland_extensions_opt      = "wayland-extensions";
char const* const mo::add_wayland_extensions_opt  = "add-wayland-extensions";
char const* const mo::drop_wayland_extensions_opt = "drop-wayland-extensions";
char const* const mo::idle_timeout_opt            = "idle-timeout";
char const* const mo::idle_timeout_when_locked_opt = "idle-timeout-when-locked";

char const* const mo::off_opt_value = "off";
char const* const mo::log_opt_value = "log";
char const* const mo::lttng_opt_value = "lttng";

char const* const mo::platform_display_libs = "platform-display-libs";
char const* const mo::platform_rendering_libs = "platform-rendering-libs";
char const* const mo::platform_input_lib = "platform-input-lib";
char const* const mo::platform_path = "platform-path";

char const* const mo::console_provider = "console-provider";
char const* const mo::logind_console = "logind";
char const* const mo::vt_console = "vt";
char const* const mo::null_console = "none";
char const* const mo::auto_console = "auto";

char const* const mo::vt_option_name = "vt";
char const* const mo::vt_switching_option_name = "vt-switching";


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
        "Command-line options (e.g. \"--wayland-extensions=wl_shell\").\n\n"
        "Environment variables capitalise long form with prefix \"MIR_SERVER_\" and \"_\" in place of \"-\".\n"
        "(E.g. \"MIR_SERVER_WAYLAND_EXTENSIONS=wl_shell\")\n\n"};

    if (program)
        result = std::string{"usage: "} + program + " [options]\n\n" + result;

    if (!config_file.empty())
        result +=
        "Config file entries are long form (e.g. \"wayland-extensions=wl_shell\").\n"
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
        (arw_server_socket_opt, "Make server Wayland socket readable and writeable by all users. "
            "For debugging purposes only.")
        (platform_display_libs, po::value<std::string>(),
            "Comma separated list of libraries to use for platform output support, e.g. mir:x11,mir:wayland. "
            "If not provided the libraries are autodetected.")
        (platform_rendering_libs, po::value<std::string>(),
            "Comma separated list of libraries to use for platform rendering support, e.g. mir:egl-generic. "
            "If not provided the libraries are autodetected.")
        (platform_input_lib, po::value<std::string>(),
            "Library to use for platform input support, e.g. mir:stub-input. "
            "If not provided this is autodetected.")
        (platform_path, po::value<std::string>()->default_value(MIR_SERVER_PLATFORM_PATH),
            "Directory to look for platform libraries.")
        (enable_input_opt, po::value<bool>()->default_value(enable_input_default),
            "Enable input.")
        (compositor_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "Configure compositor reporting. [{off,log,lttng}]")
        (display_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "Configure display reporting. [{off,log,lttng}]")
        (input_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "Configure input reporting. [{off,log,lttng}]")
        (seat_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "Configure seat reporting. [{off,log}]")
        (scene_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "Configure scene reporting. [{off,log,lttng}]")
        (shared_library_prober_report_opt, po::value<std::string>()->default_value(log_opt_value),
            "Configure shared library prober reporting. [{log,off,lttng}]")
        (shell_report_opt, po::value<std::string>()->default_value(off_opt_value),
            "Configure shell reporting. [{off,log}]")
        (composite_delay_opt, po::value<int>()->default_value(0),
            "Number of milliseconds to wait for new frames from clients before compositing. "
            "Higher values result in lower latency but risk causing frame skipping.")
        (touchspots_opt,
            "Enable visual feedback of touch events. "
            "Useful for screencasting.")
        (cursor_opt,
            po::value<std::string>()->default_value("auto"),
            "Cursor type:\n"
            " - auto: use hardware if available, or fallback to software.\n"
            " - null: cursor disabled.\n"
            " - software: always use software cursor.")
        (enable_key_repeat_opt, po::value<bool>()->default_value(true),
            "Enable server generated key repeat.")
        (idle_timeout_opt, po::value<int>()->default_value(0),
            "Number of seconds Mir will remain idle before turning off the display "
            "when the session is not locked, or 0 to keep display on forever.")
        (idle_timeout_when_locked_opt, po::value<int>()->default_value(0),
            "Number of seconds Mir will remain idle before turning off the display "
            "when the session is locked, or 0 to keep the display on forever.")
        (fatal_except_opt, "Throw an exception when a fatal error condition occurs. "
            "This replaces the default behaviour of dumping core and can make it easier to diagnose issues.")
        (debug_opt, "Enable debugging information. "
            "Useful when developing Mir servers.")
        (console_provider,
            po::value<std::string>()->default_value("auto"),
            "Method used to handle console-related tasks (device handling, VT switching, etc):\n"
            " - logind: use logind.\n"
            " - vt: use the Linux VT subsystem. Requires root.\n"
            " - none: support no console-related tasks. Useful for nested platforms which do not "
            "need raw device access and which don't have a VT concept.\n"
            " - auto: detect the appropriate provider.")
        (vt_option_name,
            boost::program_options::value<int>()->default_value(0),
            "VT to run on or 0 to use current. "
            "Only used when --console-provider=vt.")
        (vt_switching_option_name,
            boost::program_options::value<bool>()->default_value(true),
            "Enable VT switching on Ctrl+Alt+F*. "
            "Only used when --console-provider=vt|logind.");

        add_platform_options();
}

namespace
{
auto option_header_for(mir::SharedLibrary const& module) -> std::string
{
    auto const describe_module = module.load_function<mir::graphics::DescribeModule>("describe_graphics_module", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    auto const description = describe_module();

    return std::format("Options for module {}", description->name);
}
}

void mo::DefaultConfiguration::add_platform_options()
{
    namespace po = boost::program_options;
    po::options_description program_options;
    program_options.add_options()
        (platform_path,
         po::value<std::string>()->default_value(MIR_SERVER_PLATFORM_PATH),
        "");
    mo::ProgramOption options;
    options.parse_arguments(program_options, argc, argv);

    try
    {
        auto env_libpath = ::getenv("MIR_SERVER_PLATFORM_PATH");

        platform_libraries =
            [env_libpath, &options]()
            {
                    // TODO: When platform libraries have been manually specified, load only those options
                    mir::logging::NullSharedLibraryProberReport null_report;
                    auto const plugin_path = env_libpath ? env_libpath : options.get<std::string>(platform_path);
                    return mir::libraries_for_path(plugin_path, null_report);
            }();

        for (auto& platform : platform_libraries)
        {
            try
            {
                auto add_platform_options = platform->load_function<mir::graphics::AddPlatformOptions>("add_graphics_platform_options", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                auto [options, inserted] = module_options_desc.emplace(
                    platform->get_handle(),
                    option_header_for(*platform));

                // *Pretty* sure this is impossible, but let's be sure!
                assert(inserted && "Attempted to add platform options twice for the same module");

                add_platform_options(options->second);
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

std::shared_ptr<mo::Option> mo::DefaultConfiguration::global_options() const
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

auto mo::DefaultConfiguration::options_for(SharedLibrary const& module) const -> std::shared_ptr<Option>
{
    auto parsed_options = module_options[module.get_handle()];
    if (!parsed_options)
    {
        auto options = std::make_shared<ProgramOption>();
        auto& module_option_desc = module_options_desc.at(module.get_handle());

        /* We can't combine the parsed global options with a parse of the module-specific options,
         * but we *can* combine the global options description with the module options description
         * and (re)parse everything against that.
         */
        boost::program_options::options_description joint_desc;
        joint_desc.add(*program_options).add(module_option_desc);

        parse_arguments(joint_desc, *options, argc, argv);
        parse_environment(joint_desc, *options);
        parse_config_file(joint_desc, *options);

        module_options[module.get_handle()] = options;
        parsed_options = options;
    }

    return parsed_options;
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
            ("help,h", "Show command line help.");
        desc.add_options()
            ("version,V", "Display Mir version and exit.");

        options.parse_arguments(desc, argc, argv);

        auto unparsed_arguments = options.unparsed_command_line();
        std::vector<char const*> tokens;
        for (auto const& token : unparsed_arguments)
            tokens.push_back(token.c_str());

        auto const argv0 = argc > 0 ? argv[0] : "fake argv[0]";
        // Maybe the unparsed tokens are for a module?
        for (auto const& [_, module_option_desc] : module_options_desc)
        {
            if (tokens.empty())
            {
                // There aren't any unparsed arguments, so we're done here
                break;
            }

            /* We assume that argv[0] is *not* an option; instead, it's the name of the binary
             * So when reparsing, we need to add a fake argv[0]
             */
            tokens.insert(tokens.begin(), argv0);
            mo::ProgramOption tmp_options;
            tmp_options.parse_arguments(module_option_desc, tokens.size(), tokens.data());

            tokens.clear();
            unparsed_arguments = tmp_options.unparsed_command_line();
            for (auto const& token : unparsed_arguments)
            {
                tokens.push_back(token.c_str());
            }
        }

        if (!tokens.empty()) unparsed_arguments_handler(tokens.size(), tokens.data());

        if (options.is_set("help"))
        {
            std::ostringstream help_text;
            help_text << desc;
            for (auto& [_, module_desc] : module_options_desc)
            {
                // We don't want to print out headers for platforms that don't have any options
                if (!module_desc.options().empty())
                {
                    help_text << module_desc;
                }
            }
            std::cout << help_text.str() << std::endl;
        }

        if (options.is_set("version"))
        {
            std::cout << MIR_VERSION_MAJOR << "." << MIR_VERSION_MINOR << "." << MIR_VERSION_MICRO << std::endl;
        }
    }
    catch (po::error const& error)
    {
        std::ostringstream help_text;
        help_text << "Failed to parse command line options: " << error.what() << "." << std::endl << desc;
        BOOST_THROW_EXCEPTION(mir::AbnormalExit(help_text.str()));
    }
}

namespace
{
auto is_an_option(boost::program_options::options_description const& options, std::string const& name) -> bool
{
    return std::find_if(
        options.options().cbegin(),
        options.options().cend(),
        [&name](auto const& option) -> bool
        {
            return option->long_name() == name;
        }) != options.options().cend();
}
}

void mo::DefaultConfiguration::parse_environment(
    boost::program_options::options_description& desc,
    mo::ProgramOption& options) const
{
    options.parse_environment(
        desc,
        [=, this](std::string const& from) -> std::string
        {
            auto const prefix = "MIR_SERVER_";
            auto const sizeof_prefix = strlen(prefix);

            if (!from.starts_with(prefix))
            {
                return std::string{};
            }

            std::string result(from, sizeof_prefix);

            for(auto& ch : result)
            {
                if (ch == '_') ch = '-';
                else ch = std::tolower(ch, std::locale::classic()); // avoid current locale
            }

            /* Now, we want to not fail on any options that are module-specific but
             * not for *this* module (or for the global parse)
             *
             * So, we check:
             * 1. Is the result an option in `desc` (ie: an option we're trying to parse)? Return it. Or,
             * 2. Is the option in one of the module-specific option descriptors? Ignore it.
             * 3. Otherwise, return it as normal, and cause a failure
             */

            if (is_an_option(desc, result))
            {
                return result;
            }

            // Is it an option for a different module?
            for (auto const& [_, module_opt_desc] : module_options_desc)
            {
                if (is_an_option(module_opt_desc, result))
                {
                    // We want to ignore this for now, rather than raise an unknown option error
                    return std::string{};
                }
            }

            return result;
        });
}

void mo::DefaultConfiguration::parse_config_file(
    boost::program_options::options_description& desc,
    mo::ProgramOption& options) const
{
    if (!config_file.empty())
        options.parse_file(desc, config_file);
}
