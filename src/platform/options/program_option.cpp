/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/options/program_option.h"
#include "mir/log.h"

#include <boost/program_options/parsers.hpp>

#include <fstream>
#include <locale>

namespace mo = mir::options;
namespace po = boost::program_options;

namespace
{
    std::string parse_name(std::string name)
    {
        return name.substr(0, name.find_first_of(','));
    }
}

mo::ProgramOption::ProgramOption()
{
}

void mo::ProgramOption::parse_arguments(
    po::options_description const& desc,
    int argc,
    char const* argv[])
{
    // Passing argc and argv directly to command_line_parser() should do same thing. Unfortunately, when argc is 0, boost
    // 1.67+ does not correctly construct the vector and throws a std::bad_alloc
    std::vector<std::string> args{argv + 1, argv + std::max(argc, 1)};
    auto parsed_command_line = po::command_line_parser(args).options(desc).allow_unregistered().run();
    po::store(parsed_command_line, options);
    po::notify(options);

    unparsed_tokens = collect_unrecognized(parsed_command_line.options, po::include_positional);
}

std::vector<std::string> mo::ProgramOption::unparsed_command_line() const
{
    return unparsed_tokens;
}

void mo::ProgramOption::parse_environment(
    po::options_description const& desc,
    char const* prefix)
{
    auto parsed_options = po::parse_environment(
        desc,
        [=](std::string const& from) -> std::string
        {
             auto const sizeof_prefix = strlen(prefix);

             if (from.length() < sizeof_prefix || 0 != from.find(prefix)) return std::string();

             std::string result(from, sizeof_prefix);

             for(auto& ch : result)
             {
                 if (ch == '_') ch = '-';
                 else ch = std::tolower(ch, std::locale::classic()); // avoid current locale
             }

             return result;
        });

    po::store(parsed_options, options);
}

void mo::ProgramOption::parse_file(
    po::options_description const& config_file_desc,
    std::string const& name)
{
    std::string config_roots;

    if (auto config_home = getenv("XDG_CONFIG_HOME"))
        (config_roots = config_home) += ":";
    else if (auto home = getenv("HOME"))
        (config_roots = home) += "/.config:";

    if (auto config_dirs = getenv("XDG_CONFIG_DIRS"))
        config_roots += config_dirs;
    else
        config_roots += "/etc/xdg";

    std::istringstream config_stream(config_roots);

    /* Read options from config files */
    for (std::string config_root; getline(config_stream, config_root, ':');)
    {
        auto const& filename = config_root + "/" + name;

        try
        {
            std::ifstream file(filename);
            po::store(po::parse_config_file(file, config_file_desc, true), options);
        }
        catch (const po::error& error)
        {
            log_warning("Error in %s: %s", filename.c_str(), error.what());
        }
    }

    po::notify(options);
}

bool mo::ProgramOption::is_set(char const* name) const
{
    return options.count(parse_name(name));
}


bool mo::ProgramOption::get(char const* name, bool default_) const
{
    auto const parsed_name = parse_name(name);
    if (options.count(parsed_name))
    {
        return options[parsed_name].as<bool>();
    }

    return default_;
}

std::string mo::ProgramOption::get(char const* name, char const* default_) const
{
    auto const parsed_name = parse_name(name);
    if (options.count(parsed_name))
    {
        return options[parsed_name].as<std::string>();
    }

    return default_;
}

int mo::ProgramOption::get(char const* name, int default_) const
{
    auto const parsed_name = parse_name(name);
    if (options.count(parsed_name))
    {
        return options[parsed_name].as<int>();
    }

    return default_;
}

boost::any const& mo::ProgramOption::get(char const* name) const
{
    auto const parsed_name = parse_name(name);
    if (options.count(parsed_name))
    {
        return options[parsed_name].value();
    }

    static boost::any const default_;

    return default_;
}
