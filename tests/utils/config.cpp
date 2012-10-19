/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test/test_utils_config.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>

#include <fstream>

namespace mt = mir::test;

mt::Config::Config()
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

    namespace po = boost::program_options;

    po::options_description config_file_desc("Config file options");

    po::options_description env_desc("Environment options");
    env_desc.add_options()
        ("tests_use_real_graphics", po::value<bool>(), "use real graphics in tests");

    /* Read options from config files */
    for (std::string config_root; getline(config_stream, config_root, ':');)
    {
        auto const& filename = config_root + "/ubuntu_display_server/uds.config";

        try
        {
            std::ifstream file(filename);
            po::store(po::parse_config_file(file, config_file_desc), options, true);
        }
        catch (const po::error& error)
        {
            std::cerr << "ERROR in " << filename << ": " << error.what() << std::endl;
        }
    }

    /* Read options from environment */
    try
    {
        po::store(po::parse_environment(env_desc, "MIR_"), options, true);
    }
    catch (const po::error& error)
    {
        std::cerr << "ERROR in environment variables: " << error.what() << std::endl;
    }

    po::notify(options);
}

bool mt::Config::use_real_graphics()
{
    bool use_real_graphics_value{false};

    if (options.count("tests_use_real_graphics"))
        use_real_graphics_value = options["tests_use_real_graphics"].as<bool>();

    return use_real_graphics_value;
}
